#include "bril/mypackage/DataAnalyzer.h" 
#include "bril/mypackage/Events.h"
#include "bril/mypackage/exception/Exception.h"
#include "xdata/InfoSpaceFactory.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTMLClasses.h"
#include "b2in/nub/Method.h"
#include "toolbox/task/WorkLoop.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/mem/HeapAllocator.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/AutoReference.h"
#include "xcept/tools.h"
#include "interface/bril/BCM1FTopics.hh"
#include "toolbox/task/Guard.h"

XDAQ_INSTANTIATOR_IMPL (bril::mypackage::DataAnalyzer)

namespace bril{
  namespace mypackage{
    DataAnalyzer::DataAnalyzer( xdaq::ApplicationStub* s ) throw (xdaq::exception::Exception) : xdaq::Application(s),xgi::framework::UIManager(this),eventing::api::Member(this),m_applock(toolbox::BSem::FULL){
      
      xgi::framework::deferredbind(this, this, &bril::mypackage::DataAnalyzer::Default, "Default");
      b2in::nub::bind(this, &bril::mypackage::DataAnalyzer::onMessage);
      
      m_bus.fromString("brildata");
      m_fillnum = 0;
      m_runnum = 0;
      m_lsnum = 0;
      m_nbnum = 0;
      m_canpublish = false;

      getApplicationInfoSpace()->fireItemAvailable("bus",&m_bus );      
      this->getApplicationInfoSpace()->addListener(this, "urn:xdaq-event:setDefaultValues");
      
      //create workloops
      toolbox::net::URN publishlumiwlurn( "publishlumi_wl", getApplicationDescriptor()->getURN() );
      m_publishlumi_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(publishlumiwlurn.toString() ,"waiting");
      m_publishlumi_wl_as = toolbox::task::bind(this,&bril::mypackage::DataAnalyzer::publishlumi ,"publishlumi");
      
      toolbox::net::URN publishbkgwlurn( "publishbkg_wl", getApplicationDescriptor()->getURN() );
      m_publishbkg_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(publishbkgwlurn.toString() ,"waiting");
      m_publishbkg_wl_as = toolbox::task::bind(this,&bril::mypackage::DataAnalyzer::publishbkg,"publishbkg");

      //create application memory pool for outgoing messages
      toolbox::net::URN memurn( "toolbox-mem-pool",getApplicationDescriptor()->getURN() );
      try{
	toolbox::mem::HeapAllocator* allocator = new toolbox::mem::HeapAllocator();//new here does NOT require delete because this allocator is managed by the system
	m_memPool = toolbox::mem::getMemoryPoolFactory()->createPool(memurn,allocator);
      }catch(xcept::Exception& e){
	std::stringstream msg;
	msg<<"Failed to setup application memory pool "<<stdformat_exception_history(e);
	LOG4CPLUS_FATAL(getApplicationLogger(),msg.str());
	XCEPT_RAISE( bril::mypackage::exception::Exception, e.what() ); 
      }
    }
    
    void DataAnalyzer::actionPerformed(xdata::Event& e){
      LOG4CPLUS_DEBUG(getApplicationLogger(), "Received xdata event " << e.type());
      if( e.type() == "urn:xdaq-event:setDefaultValues" ){
	this->getEventingBus(m_bus.value_).addActionListener(this);
	this->addActionListener(this);
	this->getEventingBus(m_bus.value_).subscribe("beam");
	this->getEventingBus(m_bus.value_).subscribe("bcm1fagghist");

	m_publishlumi_wl->activate();
	m_publishbkg_wl->activate();
      }
      
    }
    
    void DataAnalyzer::actionPerformed(toolbox::Event& e){
      LOG4CPLUS_DEBUG(this->getApplicationLogger(), "Received toolbox event " + e.type());       
      if ( e.type() == "eventing::api::BusReadyToPublish" ){
	m_canpublish = true;
      }else if( e.type() == "urn:bril-mypackage-event:LumiSectionChanged" ){
	m_publishlumi_wl->submit( m_publishlumi_wl_as );	
      }else if( e.type() == "urn:bril-mypackage-event:NB4Changed" ){
	m_publishbkg_wl->submit( m_publishbkg_wl_as );	    
      }
    }

    void DataAnalyzer::onMessage( toolbox::mem::Reference* ref, xdata::Properties& plist ) throw ( b2in::nub::exception::Exception ){

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

	}else if( topic == "bcm1fagghist" ){

	  interface::bril::DatumHead* thead = (interface::bril::DatumHead*)(ref->getDataLocation());   
	  unsigned int previousrun = m_runnum;
	  unsigned int previousls = m_lsnum;
	  unsigned int previousnb = m_nbnum;

	  m_fillnum = thead->fillnum;
	  m_runnum = thead->runnum;
	  m_lsnum = thead->lsnum;	  
	  m_nbnum = thead->nbnum;	  
	  m_timestampsec = thead->timestampsec;
	  m_timestampmsec = thead->timestampmsec;

	  unsigned int algoid = thead->getAlgoID();
	  unsigned int channelid = thead->getChannelID();
	 
	  if( algoid != interface::bril::BCM1FAggAlgos::SUM ){	  
	    return;
	  }	  

	  interface::bril::bcm1fagghistT* agghist = (interface::bril::bcm1fagghistT*)(ref->getDataLocation());   

	  std::vector<unsigned short> perchanneldata(3564,0);	   
	  for(size_t i=0; i<3564; ++i){
	    perchanneldata[i] = agghist->payload()[i];
	  }

	  if( channelid<=16 ){
	    if( m_mylumihist.find(channelid)==m_mylumihist.end() ){
	      {//begin guard: scoped lock here is not really necessary when 100ms gap is enough to set the data provider and consumer apart
		toolbox::task::Guard<toolbox::BSem> g(m_applock);	
		m_mylumihist.insert( std::make_pair(channelid,perchanneldata) );
	      }//end guard
	    }
	  }else if( channelid>16&&channelid<33 ){
	    if( m_mybkghist.find(channelid)==m_mybkghist.end() ){
	      {//begin guard: scoped lock here is not really necessary when 100ms gap is enough  to set the data provider and consumer apart
		toolbox::task::Guard<toolbox::BSem> g(m_applock);	
		m_mybkghist.insert( std::make_pair(channelid,perchanneldata) );
	      }//end guard
	    }
	  }

	  if( previousls!=0 && (m_nbnum!=previousnb ) ){
	    bril::mypackage::NB4ChangedEvent myevent;
	    this->fireEvent( myevent );
	  }
	  
	  if( previousls!=0 && (m_lsnum!=previousls ) ){
	    bril::mypackage::LumiSectionChangedEvent myevent;
	    this->fireEvent( myevent );
	  }
	  
	}
      }      
    }

    bool DataAnalyzer::publishlumi( toolbox::task::WorkLoop* wl ){
      usleep(100000);
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
	plist.setProperty("PAYLOAD_DICT",interface::bril::bcm1fbkgT::payloaddict());
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
	
      }      
      return false;
    }
    
    bool DataAnalyzer::publishbkg( toolbox::task::WorkLoop* wl ){
      usleep(100000);
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
      }
      return false;
    }
    
    void DataAnalyzer::Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception){}
    
    DataAnalyzer::~DataAnalyzer(){}
  } // end namespace mypackage
}// end namespace bril
