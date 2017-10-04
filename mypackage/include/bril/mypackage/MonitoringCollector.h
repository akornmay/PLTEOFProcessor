#ifndef _bril_mypackage_MonitoringCollector_h_
#define _bril_mypackage_MonitoringCollector_h_

#include "xdaq/Application.h"
#include "xgi/framework/UIManager.h"
#include "xgi/framework/Method.h"
#include "xgi/exception/Exception.h"
#include "xdata/InfoSpace.h"
#include "xdata/ActionListener.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"

namespace bril{
  namespace mypackage{

    /**
       A xdaq application 
       collects and reports the monitoring infospace change "Pushed" to it;
       the web callback monitors the status of the DataSource infospace in "Pull" mode      
     */

    class MonitoringCollector : public xdaq::Application, public xgi::framework::UIManager, public xdata::ActionListener{
    public:
      //macro helper for object creation
      XDAQ_INSTANTIATOR(); 
      // constructor
      MonitoringCollector (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
      // destructor
      ~MonitoringCollector ();
      // xgi(web) callbacks
      void Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);

      // xdata::Event callback
      virtual void  actionPerformed(xdata::Event& e);

    private:
      //configuration parameters
      xdata::String m_monitoredInfoSpaceName;     

    private:
      MonitoringCollector(const MonitoringCollector&);
      MonitoringCollector& operator=(const MonitoringCollector&);
      
    }; //end class MonitoringCollector
  }// end namespace mypackage
}// end namespace bril
#endif
