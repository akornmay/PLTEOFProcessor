#include "bril/mypackage/MonitoringCollector.h" 
#include "bril/mypackage/exception/Exception.h"
#include "xdata/InfoSpaceFactory.h"
#include "xdata/ItemGroupEvent.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTMLClasses.h"


XDAQ_INSTANTIATOR_IMPL (bril::mypackage::MonitoringCollector)

namespace bril{
  namespace mypackage{
    MonitoringCollector::MonitoringCollector( xdaq::ApplicationStub* s ) throw (xdaq::exception::Exception) : xdaq::Application(s),xgi::framework::UIManager(this){

      xgi::framework::deferredbind(this, this, &bril::mypackage::MonitoringCollector::Default, "Default");           
      
      // declare the configuration parameters in the application infospace      
      getApplicationInfoSpace()->fireItemAvailable("moninfospacename",&m_monitoredInfoSpaceName);
      
      //this application itself is a subscriber to the application infospace xdata::event setDefaultValues
      this->getApplicationInfoSpace()->addListener(this, "urn:xdaq-event:setDefaultValues");
                
      std::string myapplicationurl = getApplicationContext()->getContextDescriptor()->getURL();
      myapplicationurl = myapplicationurl+"/"+getApplicationDescriptor()->getURN();      
      LOG4CPLUS_INFO( getApplicationLogger(),"MonitoringCollector URL "+myapplicationurl );

    }
    
    MonitoringCollector::~MonitoringCollector(){}
    
    void MonitoringCollector:: actionPerformed(xdata::Event& e){
      LOG4CPLUS_DEBUG(getApplicationLogger(), "Received xdata event " << e.type());
      if( e.type() == "urn:xdaq-event:setDefaultValues" ){	
	// Find monitored infospace in the context  
	// We want to collect information from all infospace instances in this context whose name contains/matches configured infospace name pattern
	// The number of infospaces can be more than one depending on how we configure the context, therefore the search result is a map
	//We attach ourselves to ItemGroupChanged event in all found infospaces
	if( m_monitoredInfoSpaceName.value_.empty() ){
	  XCEPT_RAISE(bril::mypackage::exception::Exception,"moninfospacename is not configured");
	}
	std::map<std::string, xdata::Serializable * > spaces = xdata::getInfoSpaceFactory()->match(m_monitoredInfoSpaceName.value_);	
	for(std::map<std::string, xdata::Serializable * >::iterator it = spaces.begin(); it != spaces.end(); ++it){
	  xdata::InfoSpace* is = dynamic_cast<xdata::InfoSpace *>(it->second);
	  is->addGroupChangedListener(this); 
	}      
      }//end  setDefaultValues callback

      if( e.type() == "urn:xdata-event:ItemGroupChangedEvent" ){
	std::map< std::string, xdata::Serializable* > metrics = (dynamic_cast<xdata::ItemGroupEvent&>(e)).getItems();
	xdata::InfoSpace* is = (dynamic_cast<xdata::ItemGroupEvent&>(e)).infoSpace();
	std::stringstream ss; 
	ss<<"MonitoringCollector Received monitoring from "<<is->name()<<": ";
	for( std::map<std::string, xdata::Serializable* >::iterator it=metrics.begin(); it!=metrics.end(); ++it){
	  ss<<it->first<<" "<<it->second->toString()<<" ";
	}
	ss<<std::endl;
	LOG4CPLUS_INFO(getApplicationLogger(),ss.str());   
      }
    }
    
    void MonitoringCollector::Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception){            
      }

  } // end namespace mypackage
}// end namespace bril
