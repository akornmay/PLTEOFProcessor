#include "bril/mypackage/SmartDataSource.h" 
#include "bril/mypackage/Events.h"
#include "bril/mypackage/exception/Exception.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTMLClasses.h"
#include "xcept/tools.h"

XDAQ_INSTANTIATOR_IMPL (bril::mypackage::SmartDataSource)

namespace bril{
  namespace mypackage{
    SmartDataSource::SmartDataSource( xdaq::ApplicationStub* s ) throw (xdaq::exception::Exception) : xdaq::Application(s),xgi::framework::UIManager(this){

      xgi::framework::deferredbind(this, this, &bril::mypackage::SmartDataSource::Default, "Default");

      m_pushintervalsec = 1;      
      m_popintervalsec = m_pushintervalsec*2;
      m_almostempty_threshold = 0.2;
      m_almostfull_threshold = 0.8;
      
      toolbox::net::URN popwlurn( "Popping_wl", getApplicationDescriptor()->getURN() );
      m_popping_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(popwlurn.toString() ,"waiting");
      m_popping_wl_as = toolbox::task::bind(this,&bril::mypackage::SmartDataSource::popping ,"popping");
      
      toolbox::net::URN pushwlurn( "Pushing_wl", getApplicationDescriptor()->getURN() );
      m_pushing_wl = toolbox::task::getWorkLoopFactory()->getWorkLoop(pushwlurn.toString() ,"waiting");
      m_pushing_wl_as = toolbox::task::bind(this,&bril::mypackage::SmartDataSource::pushing,"pushing");

      getApplicationInfoSpace()->fireItemAvailable("maxqueuesize",&m_maxqueuesize );
      getApplicationInfoSpace()->fireItemAvailable("pushintervalsec",&m_pushintervalsec );
      
      this->getApplicationInfoSpace()->addListener(this, "urn:xdaq-event:setDefaultValues");            
      this->addActionListener(this);//register as listener to my own events
 
      std::string myapplicationurl = getApplicationContext()->getContextDescriptor()->getURL();
      myapplicationurl = myapplicationurl+"/"+getApplicationDescriptor()->getURN();  
      LOG4CPLUS_INFO( getApplicationLogger(), "myapplicationurl "+myapplicationurl );
    }
    
    SmartDataSource::~SmartDataSource(){}
    
    void SmartDataSource::actionPerformed(xdata::Event& e){
      LOG4CPLUS_DEBUG(getApplicationLogger(), "Received xdata event " << e.type());
      if( e.type() == "urn:xdaq-event:setDefaultValues" ){
	m_queue.resize(m_maxqueuesize.value_);
	//register workloop
	m_popping_wl->activate();
	m_pushing_wl->activate();
	//submit workloop
	m_popping_wl->submit(m_popping_wl_as);
	m_pushing_wl->submit(m_pushing_wl_as);
      }
    }
    
    void SmartDataSource::actionPerformed(toolbox::Event& e){
      LOG4CPLUS_DEBUG(getApplicationLogger(), "Received toolbox event " << e.type());
      std::stringstream ss;
      if( e.type() == "urn:bril-mypackage-event:QueueAlmostEmpty" ){
	m_popintervalsec = m_pushintervalsec.value_*2; 
	ss<<"QueueAlmostEmpty, Decelerate popping to every "<<m_popintervalsec<<" seconds";
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );
      }else if ( e.type() == "urn:bril-mypackage-event:QueueAlmostFull" ){
	m_popintervalsec = m_pushintervalsec.value_/2;
	ss<<"QueueAlmostFull, Accelerate popping to every "<<m_popintervalsec<<" seconds";
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );
      }else if( e.type() == "urn:bril-mypackage-event:QueueEmpty" ){
	//In general, catch exception without handling it but just some printout is bad! - Swallow exception
	LOG4CPLUS_ERROR( getApplicationLogger(), "Queue is Empty");
	//
	//raise exception of my type 
	//
	//XCEPT_RAISE( bril::mypackage::exception::QueueEmpty, "Queue is Empty" );
	//
	//do not throw , keeping the program flow, but notify error collector
	//
	XCEPT_DECLARE( exception::Exception,myerrorobj,"Queue is Empty" );
	this->notifyQualified("error",myerrorobj);

      }else if( e.type() == "urn:bril-mypackage-event:QueueOverflow"){
	//In general, catch exception without handling it but just some printout is bad! - Swallow exception
	LOG4CPLUS_ERROR( getApplicationLogger(), "Queue overflow") ;	
	//
	//rethrow exception as my type 
	LOG4CPLUS_ERROR( getApplicationLogger(), xcept::stdformat_exception(dynamic_cast<bril::mypackage::QueueOverflowEvent&>(e).exp_) );
	//XCEPT_RETHROW( bril::mypackage::exception::QueueOverflow, "Queue Overflow", dynamic_cast<bril::mypackage::QueueOverflowEvent&>(e).exp_);

	//do not throw, keeping the program flow, but notify error collector
	XCEPT_DECLARE_NESTED( exception::Exception,myerrorobj,"Queue Overflow", dynamic_cast<bril::mypackage::QueueOverflowEvent&>(e).exp_);
	this->notifyQualified( "error",myerrorobj );
   
      }
    }

    void SmartDataSource::Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception){
      
      *out << "<h2 align=\"center\"> Page URL "<<getApplicationContext()->getContextDescriptor()->getURL()<<"</h2>";
      *out << cgicc::br();
      *out<<"<form method=\"POST\" action=\""<<"\">";
      *out<<"<input type=\"submit\" name=\"Pop\" value=\"Pop\" /> ";
      *out<<"<input type=\"submit\" name=\"Push\" value=\"Push\" />";
      *out<<"</form>";
      *out << cgicc::br();

      //process cgi input data

      cgicc::Cgicc formData(in);
      std::vector<cgicc::FormEntry> allelements = formData.getElements();
      for(std::vector<cgicc::FormEntry>::iterator it=allelements.begin(); it!=allelements.end();++it){
	std::string iname = it->getName();
	if( iname=="Pop" ){
	  if( !m_queue.empty() ){
	    m_queue.pop(); //rogue pop
	  }else{
	    bril::mypackage::QueueEmptyEvent myevent;
	    this->fireEvent(myevent);
	  }
	}
	if( iname=="Push" ){ 
	  int a = 99;
	  try{
	    m_queue.push(a); //rogue push
	  }catch( toolbox::exception::QueueFull& e){
	    bril::mypackage::QueueOverflowEvent myevent(e);
	    this->fireEvent( myevent );
	  }
	}
      }    
    }

    bool SmartDataSource::pushing(toolbox::task::WorkLoop* wl){
      LOG4CPLUS_DEBUG(getApplicationLogger(), "Entering pushing");

      usleep( int(m_pushintervalsec*1000000) ); 
      
      size_t currentqueuesize = m_queue.elements();
      float occupancy =  currentqueuesize/float(m_maxqueuesize.value_);
      if( occupancy>=m_almostfull_threshold ){	
	bril::mypackage::QueueAlmostFullEvent myevent;
	this->fireEvent(myevent);
      }

      try{
	int a = 99;
	m_queue.push(a);
	std::stringstream ss;
	ss << "Pushed, queue size "<<currentqueuesize<<", occupancy "<<occupancy ;
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );
      }catch( toolbox::exception::QueueFull& e ){
	//std::cout<<"queuefull , skip one round"<<std::endl;
	//usleep( int(m_pushintervalsec*1000000) );
	bril::mypackage::QueueOverflowEvent myevent(e);
	this->fireEvent( myevent );
      }
      return true; 
    }

    bool SmartDataSource::popping(toolbox::task::WorkLoop* wl){
      LOG4CPLUS_DEBUG(getApplicationLogger(), "Entering popping");

      usleep( int(m_popintervalsec*1000000) );

      if( !m_queue.empty() ){
	m_queue.pop();
	size_t currentqueuesize = m_queue.elements();
	float occupancy = float(currentqueuesize)/float(m_maxqueuesize.value_);
	if( occupancy<=m_almostempty_threshold ){
	  bril::mypackage::QueueAlmostEmptyEvent myevent;
	  this->fireEvent( myevent );
	}
		
	std::stringstream ss;
	ss << "Popped, queue size "<<currentqueuesize<<", occupancy "<<occupancy ;
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );
      }else{
	bril::mypackage::QueueEmptyEvent myevent;
	this->fireEvent( myevent );
      }

      return true;

    }

  } // end namespace mypackage
}// end namespace bril
