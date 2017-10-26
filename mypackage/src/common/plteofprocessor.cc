#include "bril/mypackage/plteofprocessor.h" 
#include "bril/mypackage/Events.h"
#include "bril/mypackage/exception/Exception.h"
#include "xdata/InfoSpaceFactory.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTMLClasses.h"
#include "b2in/nub/Method.h"
//toolbox
#include "toolbox/task/WorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/AutoReference.h"
#include "toolbox/TimeVal.h"
#include "toolbox/task/Guard.h"
#include "toolbox/task/TimerFactory.h"

#include "xcept/tools.h"
//topics
#include "interface/bril/PLTTopics.hh"
//#include "interface/bril/BCM1FTopics.hh"
//for slink data
#include "bril/mypackage/PLTEvent.h"
#include "interface/bril/PLTSlinkTopics.hh"
#include "bril/mypackage/zmq.hpp"

#include <iostream>
#include <fstream>
#include <ctime>

XDAQ_INSTANTIATOR_IMPL (bril::mypackage::plteofprocessor)

namespace bril{
  namespace mypackage{
    plteofprocessor::plteofprocessor( xdaq::ApplicationStub* s ) throw (xdaq::exception::Exception) : xdaq::Application(s),xgi::framework::UIManager(this),eventing::api::Member(this),m_applock(toolbox::BSem::FULL){
      
      xgi::framework::deferredbind(this, this, &bril::mypackage::plteofprocessor::Default, "Default");
      b2in::nub::bind(this, &bril::mypackage::plteofprocessor::onMessage);
      
      m_bus.fromString("brildata");
      m_fillnum = 0;
      m_runnum = 0;
      m_lsnum = 0;
      m_nbnum = 0;
      m_filename = "PLTLumiZero_"; // expanded afterwards with fill number
      m_canpublish = false;

      getApplicationInfoSpace()->fireItemAvailable("bus",&m_bus );      
      getApplicationInfoSpace()->fireItemAvailable("workloopHost", &m_workloopHost);
      getApplicationInfoSpace()->fireItemAvailable("slinkHost",&m_slinkHost);  
      getApplicationInfoSpace()->fireItemAvailable("gcFile",&m_gcFile);  
      getApplicationInfoSpace()->fireItemAvailable("alFile",&m_alFile);  
      getApplicationInfoSpace()->fireItemAvailable("trackFile", &m_trackFile);
      getApplicationInfoSpace()->fireItemAvailable("pixMask",&m_pixMask);  
      getApplicationInfoSpace()->fireItemAvailable("baseDir",&m_baseDir);  // for logs, not used for now.
      getApplicationInfoSpace()->fireItemAvailable("calibVal", &m_calibVal); /// seems to be just = 1.

      try{
	this->getApplicationInfoSpace()->addListener(this, "urn:xdaq-event:setDefaultValues");
      }catch(xcept::Exception& e){
	XCEPT_RETHROW(xdaq::exception::Exception, "Failed to add Listener", e);
      }
      
      //create workloops
      /*      toolbox::net::URN publishlumiwlurn( "publishlumi_wl", getApplicationDescriptor()->getURN() );
      m_publishlumi_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(publishlumiwlurn.toString() ,"waiting");
      m_publishlumi_wl_as = toolbox::task::bind(this,&bril::mypackage::plteofprocessor::publishlumi ,"publishlumi"); */

      /*    toolbox::net::URN publishbkgwlurn( "publishbkg_wl", getApplicationDescriptor()->getURN() );
      m_publishbkg_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(publishbkgwlurn.toString() ,"waiting");
      m_publishbkg_wl_as = toolbox::task::bind(this,&bril::mypackage::plteofprocessor::publishbkg,"publishbkg"); */

      toolbox::net::URN writetofileurn( "writetofile_wl", getApplicationDescriptor()->getURN() );  
      m_writetofile_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(writetofileurn.toString() ,"waiting");    
      m_writetofile_wl_as = toolbox::task::bind(this,&bril::mypackage::plteofprocessor::writetofile ,"writetofile");

      toolbox::net::URN roc_accidentalsurn( "roc_accidentals_wl", getApplicationDescriptor()->getURN() );  
      m_roc_accidentals_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(roc_accidentalsurn.toString() ,"waiting");
      m_roc_accidentals_wl_as = toolbox::task::bind(this,&bril::mypackage::plteofprocessor::roc_accidentals,"roc_accidentals"); 

      toolbox::net::URN roc_efficiencyurn( "roc_efficiency_wl", getApplicationDescriptor()->getURN() );     
      m_roc_efficiency_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(roc_efficiencyurn.toString() ,"waiting");     
      m_roc_efficiency_wl_as = toolbox::task::bind(this,&bril::mypackage::plteofprocessor::roc_efficiency,"roc_efficiency"); 
      
      //create application memory pool for outgoing messages
      toolbox::net::URN memurn("toolbox-mem-pool",getApplicationDescriptor()->getURN());
      try{
	toolbox::mem::HeapAllocator* allocator = new toolbox::mem::HeapAllocator();//new here does NOT require delete because this allocator is managed by the system
	m_memPool = toolbox::mem::getMemoryPoolFactory()->createPool(memurn,allocator);
      }catch(xcept::Exception& e){
	std::stringstream msg;
	msg<<"Failed to setup application memory pool "<<stdformat_exception_history(e);
	LOG4CPLUS_FATAL(getApplicationLogger(),msg.str());
	XCEPT_RAISE( bril::mypackage::exception::Exception, e.what() ); 
      }
      m_timername = "lumiPLT_eofprocessortimer";

      // then there's the flashlist publication things.
    }
    
    void plteofprocessor::actionPerformed(xdata::Event& e){
      LOG4CPLUS_DEBUG(getApplicationLogger(), "Received xdata event " << e.type());
      if( e.type() == "urn:xdaq-event:setDefaultValues" ){
	this->getEventingBus(m_bus.value_).addActionListener(this);
	this->addActionListener(this);
	try{
	  this->getEventingBus(m_bus.value_).subscribe("beam");
	  this->getEventingBus(m_bus.value_).subscribe("pltlumizero");
	}catch(eventing::api::exception::Exception& e){
	  LOG4CPLUS_ERROR(getApplicationLogger(), "Failed to subscribe to topics " + stdformat_exception_history(e));
	}
	
	std::string timername("zmqstarter");
	try{
	  toolbox::TimeVal zmqStart(2,0);
	  toolbox::task::Timer * timer = toolbox::task::getTimerFactory()->createTimer(timername);
	  timer->schedule(this, zmqStart, 0, timername);
	}catch(xdata::exception::Exception& e){
	  std::stringstream err;
	  err<< "failed to start zmq timer"<<timername;
	  LOG4CPLUS_ERROR(getApplicationLogger(),err.str()+stdformat_exception_history(e));
	  XCEPT_RETHROW(bril::mypackage::exception::Exception, err.str(),e);
        }
	// m_publishlumi_wl->activate();
	//	m_publishbkg_wl->activate();
	m_writetofile_wl->activate();

      }
    }
    
    void plteofprocessor::actionPerformed(toolbox::Event& e){
      LOG4CPLUS_DEBUG(this->getApplicationLogger(), "Received toolbox event " + e.type());       
      if ( e.type() == "eventing::api::BusReadyToPublish" ){
	m_canpublish = true;
      }else if( e.type() == "urn:bril-mypackage-event:LumiSectionChanged" ){
	m_writetofile_wl->submit( m_writetofile_wl_as );
	//	m_publishlumi_wl->submit( m_publishlumi_wl_as );	
      }else if( e.type() == "urn:bril-mypackage-event:NB4Changed" ){
	//	m_publishbkg_wl->submit( m_publishbkg_wl_as );	    
      }else if( e.type() == "urn:bril-mypackage-event:FillDumped"){
		m_roc_efficiency_wl->submit( m_roc_efficiency_wl_as);
		m_roc_accidentals_wl->submit(m_roc_accidentals_wl_as);
      }
    }

    void plteofprocessor::onMessage( toolbox::mem::Reference* ref, xdata::Properties& plist ) throw ( b2in::nub::exception::Exception ){

      toolbox::mem::AutoReference refguard(ref); //IMPORTANT: guarantee ref is released when refguard is out of scope

      std::string action = plist.getProperty("urn:b2in-eventing:action");
      if ( action == "notify" && ref!=0 ){      
	std::string topic = plist.getProperty("urn:b2in-eventing:topic");
	LOG4CPLUS_DEBUG(getApplicationLogger(), "Received topic " +topic);
	if( topic == "beam" ){

	  interface::bril::DatumHead* thead = (interface::bril::DatumHead*)(ref->getDataLocation());   
	  std::string payloaddict = plist.getProperty("PAYLOAD_DICT");

	  interface::bril::CompoundDataStreamer tc(payloaddict);
	  char bstatus[28];
	  tc.extract_field( bstatus , "status", thead->payloadanchor );
	  m_beamstatus = std::string(bstatus);
	}else if( topic == "pltlumizero" ){

	  interface::bril::DatumHead* thead = (interface::bril::DatumHead*)(ref->getDataLocation());   
	  unsigned int previousfill = m_fillnum;
	  //	  unsigned int previousrun = m_runnum;
	  unsigned int previousls = m_lsnum;
	  //	  unsigned int previousnb = m_nbnum;

	  m_fillnum = thead->fillnum;
	  m_runnum = thead->runnum;
	  m_lsnum = thead->lsnum;	  
	  m_nbnum = thead->nbnum;	  
	  m_timestampsec = thead->timestampsec;
	  m_timestampmsec = thead->timestampmsec;

	  // unsigned int algoid = thead->getAlgoID();
	  // unsigned int channelid = thead->getChannelID();	 
	  //	  LOG4CPLUS_INFO(getApplicationLogger(), "Algo ID " +algoid);
	  //	  LOG4CPLUS_INFO(getApplicationLogger(), "Channel ID " +channelid);

	  // LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );	

	  // if( previousfill!=m_fillnum ){ // fill number changes, we close the previous file stream and open a new one.
	  //   if (!tofile.is_open()){ 
	  //     std::string file = m_filename;// + std::string(m_fillnum);
	  //     file+=".csv";
      	  //     tofile.open(file.c_str());
	  //	  std::stringstream info;
	  //     info << "STARTED new EOFProcessor file "<< file;
	  //     LOG4CPLUS_INFO (getApplicationLogger(), info.str());
	  //   }
	  // }

	  float avgraw = 0;
	  float avg = 0;
	  interface::bril::pltlumizeroT* lumizero = (interface::bril::pltlumizeroT*)(ref->getDataLocation());   
	  interface::bril::CompoundDataStreamer cds(interface::bril::pltlumizeroT::payloaddict());
	  cds.extract_field(&avgraw,"avgraw", lumizero->payloadanchor);
	  cds.extract_field(&avg,"avg", lumizero->payloadanchor);
	  m_avgrawlumi = avgraw;
	  m_avglumi = avg;
	  
	  ///assign an fillnumber to the lumifile
	  std::ofstream myfile;
	  if (previousfill!=0 && (m_fillnum!=previousfill)){
	    std::stringstream fn;	 
	    fn << m_fillnum <<".csv";	 
	    std::string tmp_fn = fn.str();   
	    myfile.open(tmp_fn.c_str(), std::ios::app);
	    myfile.close();
	    std::stringstream info;
	    info << "STARTED new EOFProcessor lumifile: "<< tmp_fn;
	    LOG4CPLUS_INFO (getApplicationLogger(), info.str());
	  }

	  ///turning to Stable beams : open the file, write into the stringstream (asynchronous callback), close the file : workloop every 4nb      	  
 	  if( previousls!=0 && (m_lsnum!=previousls ) ){
	    bril::mypackage::LumiSectionChangedEvent myevent;
	    this->fireEvent( myevent );
	  }
	  
	  // if( previousls!=0 && (m_lsnum!=previousls ) ){
	  //   bril::mypackage::LumiSectionChangedEvent myevent;
	  //   this->fireEvent( myevent );
	  // }
	  
	}
      }      
    }

    void plteofprocessor::stopTimer(){
      toolbox::task::Timer* timer = toolbox::task::TimerFactory::getInstance()->getTimer(m_timername);
      if( timer->isActive() ){
        timer->stop();
      }
    }
    void plteofprocessor::startTimer(){
      if(!toolbox::task::TimerFactory::getInstance()->hasTimer(m_timername)){
        (void) toolbox::task::TimerFactory::getInstance()->createTimer(m_timername);
      }
      toolbox::task::Timer* timer = toolbox::task::TimerFactory::getInstance()->getTimer(m_timername);
      toolbox::TimeInterval interval(20,0);
      try{
        stopTimer();
	toolbox::TimeVal start = toolbox::TimeVal::gettimeofday();
	timer->start();
	timer->scheduleAtFixedRate( start, this, interval, 0, m_timername);
      }catch (toolbox::task::exception::InvalidListener& e){
	LOG4CPLUS_FATAL (this->getApplicationLogger(), xcept::stdformat_exception_history(e));
      }catch (toolbox::task::exception::InvalidSubmission& e){
	LOG4CPLUS_FATAL (this->getApplicationLogger(), xcept::stdformat_exception_history(e));
      }catch (toolbox::task::exception::NotActive& e){
        LOG4CPLUS_FATAL (this->getApplicationLogger(), xcept::stdformat_exception_history(e));
      }catch (toolbox::exception::Exception& e){
	LOG4CPLUS_FATAL (this->getApplicationLogger(), xcept::stdformat_exception_history(e));
      }
    }

    void plteofprocessor::timeExpired(toolbox::task::TimerEvent & e)
    {
      LOG4CPLUS_INFO(getApplicationLogger(),"Beginning zmq listener");
      //      zmqClient();
    }

    void plteofprocessor::zmqClient()
    {
      //zmq_poll
      zmq::context_t zmq_context(1);
      //workloop listener : get TCDS from workloop
      zmq::socket_t workloop_socket(zmq_context, ZMQ_SUB);
      workloop_socket.connect(m_workloopHost.c_str());
      workloop_socket.setsockopt(ZMQ_SUBSCRIBE, 0, 0);
      
      uint32_t EventHisto[3573];
      uint32_t tcds_info[4];
      uint32_t time_orbit, orbit;
      uint32_t channel;
      int old_run = 1;
      int old_ls = 999999;
      int old_fill = 1;
      
      // begin listener : get slink data
      zmq::socket_t slink_socket(zmq_context, ZMQ_SUB);
      slink_socket.connect(m_slinkHost.c_str());
      slink_socket.setsockopt(ZMQ_SUBSCRIBE, 0, 0);
      
      uint32_t slinkBuffer[1024];
      // Translation of pixel FED channel number to readout channel number.
      int nevents = 0; // counter of slink items received
      const unsigned validChannels[] = {2, 4, 5, 8, 10, 11, 13, 14, 16, 17, 19, 20};
      const int channelNumber[37] = {-1, 0, 1, -1, 2, 3, -1, 4, 5, -1, 6, 7,  // 0-11
				     -1, 8, 9, -1, 10, 11, -1, 12, 13, -1, 14, 15, // 12-23
				     -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
      };

      // Set up poll object
      zmq::pollitem_t pollItems[2] = {
        {workloop_socket, 0, ZMQ_POLLIN, 0},
        {slink_socket, 0, ZMQ_POLLIN, 0}
      };

      //PLT EVENT
      PLTEvent *event = new PLTEvent("", m_gcFile.value_, m_alFile.value_, kBuffer);
      event->ReadOnlinePixelMask(m_pixMask.value_);
      event->SetPlaneClustering(PLTPlane::kClustering_NoClustering, PLTPlane::kFiducialRegion_All);
      event->SetPlaneFiducialRegion(PLTPlane::kFiducialRegion_All);
      event->SetTrackingAlgorithm(PLTTracking::kTrackingAlgorithm_01to2_AllCombs);
      //      event->SetTrackingAlgorithm(PLTTracking::kTrackingAlgorithm_NoTracking);
 
      // Initialize tools
      std::vector<unsigned> channels(validChannels, validChannels + sizeof(validChannels)/sizeof(unsigned));
      EventAnalyzer *eventAnalyzer = new EventAnalyzer(event, m_alFile.value_, m_trackFile.value_, channels);
  
      //to save to file the accidentals etc
      std::ofstream myslinkfile;
      std::stringstream sstream; // main stream
      //	  std::stringstream accsstream; // second one for per channel loop
      std::string tmp_fn;
      std::stringstream fn;	 

      ///Loop           
      while(1) {
	zmq_poll(&pollItems[0], 2, -1);

	// check
	if (pollItems[0].revents & ZMQ_POLLIN){
	  zmq::message_t zmq_EventHisto(3573*sizeof(uint32_t));
	  workloop_socket.recv(&zmq_EventHisto);

	  memcpy(EventHisto, zmq_EventHisto.data(), 3573*sizeof(uint32_t));
	  memcpy(&channel, (uint32_t*)EventHisto+2, sizeof(uint32_t));
	  memcpy(&tcds_info, (uint32_t*)EventHisto+3, 6*sizeof(uint32_t));
	  memcpy(EventHisto, &time_orbit, sizeof(uint32_t));
	  memcpy(&orbit, (uint32_t*)EventHisto+1, sizeof(uint32_t));

	  m_nibble = tcds_info[0];
	  m_ls = tcds_info[1];
	  m_run = tcds_info[2];
	  m_fill = tcds_info[3];
	  m_time_orbit = time_orbit;
	  m_orbit = orbit;

	  fn << "Slink" << m_fill <<".csv";	 
	  tmp_fn = fn.str();
	  
	  if (m_fill != old_fill){
	    old_fill = m_fill;
	    myslinkfile.open(tmp_fn.c_str(), std::ios::app);
	    myslinkfile.close();
	  }

	  // If we've started a new run or a new lumisection: write to file.
	  if (m_ls > old_ls or m_run > old_run) {  
	    std::time_t tunix = time(NULL);
	    tm* timePtr = localtime(&tunix);
	    //	    std::cout << "day of month" << timePtr->tm_mday << std::endl;
	    // std::time_t tslink;
	    // tm* timePtrSlink = {0};
	    int slinka = (int) m_orbit / 3564;
	    int slinkb = m_orbit % 3564;
	    //	    std::cout << m_fill<<","<<m_run<<","<<m_ls<<","<<m_nibble<<","<<m_time_orbit<<","<<m_orbit << " slinkHour = "<< slinka << ":"<< slinkb <<std::endl;
	    const unsigned nChannels = sizeof(validChannels) / sizeof(validChannels[0]);
	    std::vector<float> eff;
	    
	    // myslinkfile.open(tmp_fn.c_str(), std::ios::app);
	    // sstream <<m_fill<<","<<m_run<<","<<m_ls<<","<<m_nibble<<","<<m_time_orbit<<","<<m_orbit<<",";

	    for (unsigned i = 0; i < nChannels; ++i) {
	      eff   = eventAnalyzer->GetTelescopeEfficiency(validChannels[i]);
	      float channelEff= eff[0]*eff[1]*eff[2];
	      float acc  = eventAnalyzer->GetTelescopeAccidentals(validChannels[i]);
	      // float rate = eventAnalyzer->GetZeroCounting(validChannels[i]);
	      //	      std::cout<< channelEff<<",";	      
	      //	      std::cout << channelEff<<",";
	      // sstream << validChannels[i] <<","<< acc << ",";
	      std::cout<< eff[0] <<",";
	    }
	    // myslinkfile << sstream.str() << std::endl;
	    // myslinkfile.close();
	  }
	  old_ls = m_ls;
	  old_run = m_run;
	} // slink workloop analysis

	// if (pollItems[1].revents & ZMQ_POLLIN) {
	//   //process the event
	//   int eventSize = slink_socket.recv((void*)slinkBuffer, sizeof(slinkBuffer)*sizeof(uint32_t), 0);
	//   event->GetNextEvent(slinkBuffer, eventSize/sizeof(uint32_t));
	//   eventAnalyzer->AnalyzeEvent();
	//	}///slink event analysis
      }// message loop
    }
    bool plteofprocessor::writetofile( toolbox::task::WorkLoop* wl ){
      usleep(100000);
      
      std::ofstream myfile;
      std::stringstream thread;
      std::string tmp_fn;
      std::stringstream fn;	 
      fn << m_fillnum <<".csv";	 
      tmp_fn = fn.str();

      //write to file as soon as it is stable beams, every LS
      if (m_beamstatus == "STABLE BEAMS"){
	//tofile
	myfile.open(tmp_fn.c_str(), std::ios::app);
	thread <<m_fillnum<<","<<m_runnum<<","<<m_lsnum<<","<<m_nbnum<<","<<m_timestampsec<<","<<m_timestampmsec<<","<<m_avgrawlumi<<","<<m_avglumi;
	myfile << thread.str() << std::endl;
	myfile.close();
      }
      return false;
    }
    
    bool plteofprocessor::roc_efficiency( toolbox::task::WorkLoop* wl){
      usleep(100000);

      std::ofstream myfile;
      std::stringstream sstream;
      std::string tmp_fn;
      std::stringstream fn;
      fn <<"efficiency"<< m_fillnum <<  ".csv";
      std::string DataFileName = "Slink_20170924.111114.dat";
      std::string GainCalFile = "GainCalFits_20170518.143905.dat";
      std::string TrackDist = "TrackDistributions_MagnetOn2017_5718.txt";
      std::string Alignment = "Trans_Alignment_2017MagnetOn_Prelim.dat";
      if (m_beamstatus=="INJECTION PROBE BEAM"){
	//      myfile.open(fn.c_str(),std::ios::app);
	LOG4CPLUS_INFO(getApplicationLogger(), "Computing efficiency");	
	TrackingEfficiency(DataFileName, GainCalFile, Alignment);
	LOG4CPLUS_INFO(getApplicationLogger(), "DONE");
      }
      return false;
    }

    bool plteofprocessor::roc_accidentals( toolbox::task::WorkLoop*wl){
      usleep(100000);
      return false;
    }
    bool plteofprocessor::publishlumi( toolbox::task::WorkLoop* wl ){
      /*      usleep(100000);
      if(m_canpublish){

	float avgraw = 0;
	float avg = 0;

	float bxraw[3564];
	float bx[3564];
	memset(bxraw,0,sizeof(float)*3564);
	memset(bx,0,sizeof(float)*3564);
	
	unsigned int maskhigh = 32;
	unsigned int masklow = 12;
	size_t idx=0;

	{//begin guard: scoped lock here is not really necessary when 100ms usleep is enough to guarantee all messages have arrived
	  toolbox::task::Guard<toolbox::BSem> g(m_applock);	

	  for(std::map<unsigned int , std::vector<unsigned short> >::iterator it=m_mylumihist.begin(); it!=m_mylumihist.end();++it, ++idx){
	    avgraw +=  it->second[0];
	    avg += it->second[1];
	    bxraw[idx] += it->second[idx];
	    bx[idx] += it->second[idx]*1.6;
	  }
	  avgraw = float(avgraw)/16.;
	  avg = avgraw*1.6;	  
	  m_mylumihist.clear();
	  
	}//end guard
	
	xdata::Properties plist;
	plist.setProperty("DATA_VERSION",interface::bril::DATA_VERSION);
	if( m_beamstatus!="STABLE BEAMS"&&m_beamstatus!="SQUEEZE"&&m_beamstatus!="FLAT TOP"&&m_beamstatus!="ADJUST"){
	  plist.setProperty("NOSTORE","1");
	}
	toolbox::mem::Reference* lumiRef = 0;
	size_t totalsize  = interface::bril::bcm1flumiT::maxsize();	
	lumiRef = toolbox::mem::getMemoryPoolFactory()->getFrame(m_memPool,totalsize);	
	lumiRef->setDataSize(totalsize);
	interface::bril::DatumHead* header = (interface::bril::DatumHead*)(lumiRef->getDataLocation());
	header->setTime(m_fillnum,m_runnum,m_lsnum,m_nbnum,m_timestampsec,m_timestampmsec);
	header->setResource(interface::bril::DataSource::BCM1F,0,0,interface::bril::StorageType::COMPOUND);
	header->setFrequency(64);
	header->setTotalsize(totalsize);
	interface::bril::CompoundDataStreamer tc( interface::bril::bcm1flumiT::payloaddict() );
	std::string calibtag("test");
	tc.insert_field( header->payloadanchor, "avgraw", &avgraw );
	tc.insert_field( header->payloadanchor, "avg", &avg );
	tc.insert_field( header->payloadanchor, "calibtag", calibtag.c_str() );
	tc.insert_field( header->payloadanchor, "bxraw", bxraw );
	tc.insert_field( header->payloadanchor, "bx", bx );
	tc.insert_field( header->payloadanchor, "maskhigh", &maskhigh );
	tc.insert_field( header->payloadanchor, "masklow", &masklow );
	
	std::stringstream ss;
	ss<<"Publishing LUMI "<<m_fillnum<<","<<m_runnum<<","<<m_lsnum<<","<<m_nbnum<<" avgraw "<<avgraw<<" avg "<<avg;
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );	
	
	try{
	  getEventingBus(m_bus.value_).publish( "bcm1flumi",  lumiRef , plist); //hand over ownership, bkgRef will be disposed by the messenger
	}catch( xcept::Exception& e ){	 
	  lumiRef->release(); // IMPORTANT: This is necessary! in case of messenger problems, I must dispose lumiRef 
	  LOG4CPLUS_ERROR( getApplicationLogger(),stdformat_exception_history(e) );
	  XCEPT_DECLARE_NESTED(bril::mypackage::exception::Exception,myerrorobj,"Failed to publish bcm1flumi to "+m_bus.toString(),e);
	  notifyQualified("error",myerrorobj);
	}
	
	} */     
      return false;
    }
    
    bool plteofprocessor::publishbkg( toolbox::task::WorkLoop* wl ){
      /*      usleep(100000);
      if(m_canpublish){	

	float plusz = 0;
	float minusz = 0;	
	
	{// begin guard: scoped lock here is not really necessary when 100ms usleep is enough to guarantee all messages have arrived
	  toolbox::task::Guard<toolbox::BSem>  g(m_applock);	
	  for(std::map<unsigned int , std::vector<unsigned short> >::iterator it=m_mybkghist.begin(); it!=m_mybkghist.end();++it){
	    plusz += it->second[12];
	    minusz += it->second[14];
	  }
	  plusz = float(plusz)/16.;
	  minusz = float(minusz)/16.;		
	  m_mybkghist.clear();
	}//end guard

	toolbox::mem::Reference* bkgRef = 0;
	size_t totalsize  = interface::bril::bcm1fbkgT::maxsize();	
	bkgRef = toolbox::mem::getMemoryPoolFactory()->getFrame(m_memPool,totalsize);
	xdata::Properties plist;
	plist.setProperty("DATA_VERSION",interface::bril::DATA_VERSION);
	plist.setProperty("PAYLOAD_DICT",interface::bril::bcm1fbkgT::payloaddict());
	if( m_beamstatus!="STABLE BEAMS"&&m_beamstatus!="SQUEEZE"&&m_beamstatus!="FLAT TOP"&&m_beamstatus!="ADJUST"){
	  plist.setProperty("NOSTORE","1");
	}
	bkgRef->setDataSize(totalsize);
	interface::bril::DatumHead* header = (interface::bril::DatumHead*)(bkgRef->getDataLocation());
	header->setTime(m_fillnum,m_runnum,m_lsnum,m_nbnum,m_timestampsec,m_timestampmsec);
	header->setResource(interface::bril::DataSource::BCM1F,0,0,interface::bril::StorageType::COMPOUND);
	header->setFrequency(4);
	header->setTotalsize(totalsize);
	interface::bril::CompoundDataStreamer tc( interface::bril::bcm1fbkgT::payloaddict() );
	
	tc.insert_field( header->payloadanchor, "plusz", &plusz );
	tc.insert_field( header->payloadanchor, "minusz", &minusz );

	std::stringstream ss;
	ss<<"Publishing BKG "<<m_fillnum<<","<<m_runnum<<","<<m_lsnum<<","<<m_nbnum<<" plusz "<<plusz<<" minusz "<<minusz;
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );	
	
	try{
	  getEventingBus(m_bus.value_).publish( "bcm1fbkg",  bkgRef , plist); //hand over ownership, bkgRef will be disposed by the messenger
	}catch( xcept::Exception& e ){
	  bkgRef->release(); // IMPORTANT: This is necessary! in case of messenger problems, I must dispose bkgRef 
	  LOG4CPLUS_ERROR( getApplicationLogger(),stdformat_exception_history(e) );
	  XCEPT_DECLARE_NESTED(bril::mypackage::exception::Exception,myerrorobj,"Failed to publish bcm1fbkg to "+m_bus.toString(),e);
	  notifyQualified("error",myerrorobj);
	}
	}*/
      return false;
    }
    
    void plteofprocessor::Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception){
      *out << busesToHTML();
      std::string appurl;
      *out << "We're publishing data" ;
      *out << cgicc::br();
      *out << cgicc::table().set("class","xdaq-table-vertical");
      *out << cgicc::caption("input/output");
      *out << cgicc::tbody();

      *out << cgicc::tr();
      *out << cgicc::th("PLTLumi zero / ls");
      *out << cgicc::td(interface::bril::pltlumizeroT::topicname());
      *out << cgicc::tr();
      
      *out << cgicc::tbody();
      *out << cgicc::table();
      *out << cgicc::br() << std::endl;
    }
    
    plteofprocessor::~plteofprocessor(){}
  } // end namespace mypackage
}// end namespace bril
