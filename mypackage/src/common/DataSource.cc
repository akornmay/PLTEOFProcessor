
#include "bril/mypackage/DataSource.h" 
#include "xdata/InfoSpaceFactory.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTMLClasses.h"
#include "toolbox/task/TimerFactory.h"

XDAQ_INSTANTIATOR_IMPL (bril::mypackage::DataSource)

namespace bril{
  namespace mypackage{
    DataSource::DataSource( xdaq::ApplicationStub* s ) throw (xdaq::exception::Exception) : xdaq::Application(s),xgi::framework::UIManager(this){

      xgi::framework::deferredbind(this, this, &bril::mypackage::DataSource::Default, "Default");
      
      // set configuration parameter default values
      m_qcontentorder.fromString("increase");
      m_initqueuesize = 5;
      m_currentqueuesize = 5;
      m_timerintervalsec = 5;
      // declare the configuration parameters in the application infospace      
      getApplicationInfoSpace()->fireItemAvailable("contentorder",&m_qcontentorder );
      getApplicationInfoSpace()->fireItemAvailable("initqueuesize",&m_initqueuesize );
      getApplicationInfoSpace()->fireItemAvailable("timerintervalsec",&m_timerintervalsec );

      //this application itself is a subscriber to the application infospace xdata::event setDefaultValues
      this->getApplicationInfoSpace()->addListener(this, "urn:xdaq-event:setDefaultValues");
      
      // create monitoring infospace and declare monitored variables in the monitoring infospace     
      std::string nid("DataSource_mon");
      std::string monurn = createQualifiedInfoSpace(nid).toString();
      m_monInfoSpace = dynamic_cast<xdata::InfoSpace* >(xdata::getInfoSpaceFactory()->find(monurn));  
      m_monInfoSpace->fireItemAvailable("currentqueuesize",&m_currentqueuesize );
      m_monInfoSpace->fireItemAvailable("initqueuesize",&m_initqueuesize );           
      
      std::string myapplicationurl = getApplicationContext()->getContextDescriptor()->getURL();
      myapplicationurl = myapplicationurl+"/"+getApplicationDescriptor()->getURN();      

            
      // create timer
      toolbox::net::URN timerurn( "DataSource_timer", getApplicationDescriptor()->getURN() );
      m_timername = timerurn.toString();
      toolbox::task::TimerFactory::getInstance()->createTimer( m_timername );

      LOG4CPLUS_INFO( getApplicationLogger(),"DataSource URL "+myapplicationurl+", timer "+m_timername );
    }
    
    DataSource::~DataSource(){}
    
    void DataSource:: actionPerformed(xdata::Event& e){
      LOG4CPLUS_DEBUG(getApplicationLogger(), "Received xdata event " << e.type());
      if( e.type() == "urn:xdaq-event:setDefaultValues" ){
	//This is when the configuration xml is parsed and variables in application infospace filled
	std::stringstream ss;
	ss<<"Configured as: queue content order type "<<m_qcontentorder.toString()<<" , initial queue size "<<m_initqueuesize.toString()<<"\n";
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );
	ss.clear(); ss.str("");
	
	ss<<"Queue content: ";
	int iv = 0;
	int dv = m_initqueuesize.value_;
	for( size_t i=0; i<m_initqueuesize.value_; ++i, ++iv, --dv ){
	  if( m_qcontentorder.value_=="increase" ){	    
	    m_queue.push( iv );	    
	    ss<<iv<<" ";
	  }else{
	    m_queue.push( dv );
	    ss<<dv<<" ";
	  }
	}
	m_currentqueuesize = m_queue.elements();      	
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );

	ss.clear(); ss.str("");
	ss<<"Start timer with "<<m_timerintervalsec.toString()<<" sec frequency";
	LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );
	toolbox::task::Timer* timer = toolbox::task::TimerFactory::getInstance()->getTimer(m_timername);
	toolbox::TimeVal now = toolbox::TimeVal::gettimeofday();
	toolbox::TimeInterval period( m_timerintervalsec.value_,0 );
	timer->scheduleAtFixedRate( now, this, period, 0, "popqueue");	
      }//end  setDefaultValues callback

    }
    
    void DataSource::Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception){
      
      *out << "<h2 align=\"center\"> Page URL "<<getApplicationContext()->getContextDescriptor()->getURL()<<"</h2>";
      *out << cgicc::br();
      *out<<"<form method=\"POST\" action=\""<<"\">";
      *out<<"<input type=\"submit\" name=\"Pull\" value=\"Pull\" />";
      *out<<"</form>";
      *out << cgicc::br(); 
    
      //process cgi input data 
      cgicc::Cgicc formData(in);
      std::vector<cgicc::FormEntry> allelements = formData.getElements();
      for(std::vector<cgicc::FormEntry>::iterator it=allelements.begin(); it!=allelements.end();++it){
	std::string iname = it->getName();
	if( iname=="Pull" ){     //process monitor button to display the status of the queue	  
	  //
	  // "Pull" a group of monitoring variables from the monitoring infospace
	  // This block of code can be in any application classes of the same context because the communication is via infospace
	  // If the monitored variables are accessible as data member, they can be used directly, which is just a shorcut.
	  //
	  if( m_monInfoSpace->hasItem("initqueuesize") &&  m_monInfoSpace->hasItem("currentqueuesize") ){
	    xdata::Serializable* initqueuesize = m_monInfoSpace->find("initqueuesize");
	    xdata::Serializable* currentqueuesize =  m_monInfoSpace->find("currentqueuesize");
	    std::list<std::string> monitemlist;
	    monitemlist.push_back("initqueuesize");
	    monitemlist.push_back("currentqueuesize");
	    m_monInfoSpace->lock();
	    m_monInfoSpace->fireItemGroupRetrieve( monitemlist,this );
	    m_monInfoSpace->unlock();
	    *out << cgicc::br();
	    *out << "Current queue size : "<<currentqueuesize->toString();
	    *out << cgicc::br();
	    *out << "Initial queue size : "<<initqueuesize->toString();
	  }	  
	}
      }
  }
    
    void DataSource::timeExpired( toolbox::task::TimerEvent& e ){
      if( e.getTimerTask()->name == "popqueue"){
	if( m_queue.elements()==0 ){
	  LOG4CPLUS_INFO( getApplicationLogger(), "Queue is empty, do nothing" );
	}else{	   
	  int valuepopped = m_queue.pop();
	  m_currentqueuesize = m_queue.elements();
	  std::stringstream ss;
	  ss << "DataSource Popped value "<<valuepopped<<" , current queue size "<<m_currentqueuesize.toString() ;
	  LOG4CPLUS_INFO( getApplicationLogger(), ss.str() );
	  std::list<std::string> monitemlist;
	  monitemlist.push_back("initqueuesize");
	  monitemlist.push_back("currentqueuesize");
	  m_monInfoSpace->fireItemGroupChanged(monitemlist, this); // "Push" the status change of this group of monitoring items. 
	                                                                                                // If this monitoring matrix and its infospace are made known to the external monitoring collectors, it would be called a "flashlist".
	  
	}
      }
    }

  } // end namespace mypackage
}// end namespace bril
