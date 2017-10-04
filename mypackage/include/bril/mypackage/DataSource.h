#ifndef _bril_mypackage_DataSource_h_
#define _bril_mypackage_DataSource_h_

#include "xdaq/Application.h"
#include "xgi/framework/UIManager.h"
#include "xgi/framework/Method.h"
#include "xgi/exception/Exception.h"
#include "xdata/InfoSpace.h"
#include "xdata/ActionListener.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "toolbox/squeue.h"
#include "toolbox/task/TimerListener.h"

namespace bril{
  namespace mypackage{

    /**
       DataSource is a xdaq application with a prefilled queue.
       The initial size of the queue, the order of the queue content and the timer interval are configured via the application configuration xml
       The queue pops the element on a timer.
       On every queue size change, DataSource fires xdata changed event, i.e. to "Push" the monitoring items.
       The hyperdaq page monitors the status of the queue on a web click
     */

    class DataSource : public xdaq::Application, public xgi::framework::UIManager, public xdata::ActionListener, toolbox::task::TimerListener{
    public:
      //macro helper for object creation
      XDAQ_INSTANTIATOR(); 
      // constructor
      DataSource (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
      // destructor
      ~DataSource ();
      // xgi(web) callbacks
      void Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);

      // xdata::Event callback
      virtual void  actionPerformed(xdata::Event& e);

      // timer callback  
      virtual void timeExpired(toolbox::task::TimerEvent& e);

    private:
      //configuration parameters
      xdata::String m_qcontentorder ; //order type of the queue content "increase" or "decrease"
      
      xdata::UnsignedInteger32 m_initqueuesize ; // initial squeue size

      xdata::UnsignedInteger32 m_timerintervalsec ; //pop queue every x seconds

      //monitoring infospace
      xdata::InfoSpace* m_monInfoSpace; 

      //monitored variables
      xdata::UnsignedInteger32 m_currentqueuesize; // current number of elements remains in the squeue

      // a queue filled in the constructor and popped on web button click
      toolbox::squeue<int> m_queue;
      
      // timer name
      std::string  m_timername;
      
    private:
      DataSource(const DataSource&);
      DataSource& operator=(const DataSource&);
      
    }; //end class MonitorWebPull
  }// end namespace mypackage
}// end namespace bril
#endif
