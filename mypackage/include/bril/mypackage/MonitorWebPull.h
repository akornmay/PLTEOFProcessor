#ifndef _bril_mypackage_MonitorWebPull_h_
#define _bril_mypackage_MonitorWebPull_h_

#include "xdaq/Application.h"
#include "xgi/framework/UIManager.h"
#include "xgi/framework/Method.h"
#include "xgi/exception/Exception.h"
#include "xdata/InfoSpace.h"
#include "xdata/ActionListener.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "toolbox/squeue.h"

namespace bril{
  namespace mypackage{

    /**
       A xdaq application with a prefilled queue.
       The initial size of the queue and the order of the queue content are configured via the application configuration xml
       The queue pops the element on a web click
       The hyperdaq page monitors the status of the queue on a web click
     */

    class MonitorWebPull : public xdaq::Application, public xgi::framework::UIManager, public xdata::ActionListener{
    public:
      //macro helper for object creation
      XDAQ_INSTANTIATOR(); 
      // constructor
      MonitorWebPull (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
      // destructor
      ~MonitorWebPull ();
      // xgi(web) callbacks
      void Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);

      // xdata::Event callback
      virtual void  actionPerformed(xdata::Event& e);

    private:
      //configuration parameters
      xdata::String m_qcontentorder ; //order type of the queue content "increase" or "decrease"
      
      xdata::UnsignedInteger32 m_initqueuesize ; // initial squeue size

      //monitoring infospace
      xdata::InfoSpace* m_monInfoSpace; 

      //monitored variables
      xdata::UnsignedInteger32 m_currentqueuesize; // current number of elements remains in the squeue

      // a queue filled in the constructor and popped on web button click
      toolbox::squeue<int> m_queue;
      
    private:
      MonitorWebPull(const MonitorWebPull&);
      MonitorWebPull& operator=(const MonitorWebPull&);
      
    }; //end class MonitorWebPull
  }// end namespace mypackage
}// end namespace bril
#endif
