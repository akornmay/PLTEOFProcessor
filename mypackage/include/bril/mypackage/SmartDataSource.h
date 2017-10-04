#ifndef _bril_mypackage_SmartDataSource_h_
#define _bril_mypackage_SmartDataSource_h_

#include "xdaq/Application.h"
#include "xgi/framework/UIManager.h"
#include "xgi/framework/Method.h"
#include "xgi/exception/Exception.h"
#include "xdata/InfoSpace.h"
#include "xdata/ActionListener.h"
#include "xdata/String.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/Float.h"
#include "toolbox/squeue.h"
#include "toolbox/task/WorkLoop.h"
#include "toolbox/ActionListener.h"
#include "toolbox/EventDispatcher.h"
#include <string>

namespace bril{
  namespace mypackage{

    /**
       A xdaq application who is capable of self-regulating the queue population.
     */

    class SmartDataSource : public xdaq::Application, public xgi::framework::UIManager, public xdata::ActionListener, toolbox::ActionListener, toolbox::EventDispatcher{

    public:
      //macro helper for object creation
      XDAQ_INSTANTIATOR(); 
      // constructor
      SmartDataSource (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
      // destructor
      ~SmartDataSource ();
      // xgi(web) callbacks
      void Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);

      // xdata::Event callback
      virtual void  actionPerformed(xdata::Event& e);

      // toolbox::Event callback  
      virtual void actionPerformed(toolbox::Event& e);

    private:
      //configuration parameters
      
      xdata::UnsignedInteger32 m_maxqueuesize ; // max squeue size

      xdata::Float m_pushintervalsec; //const push queue rate

      float m_popintervalsec ; //variable, pop queue rate
      
      float m_almostempty_threshold ; // queue almost empty threshold
      float m_almostfull_threshold ; // queue almost full threshold

      // the queue
      toolbox::squeue<int> m_queue;
      
      // workloop
      toolbox::task::WorkLoop* m_popping_wl;
      toolbox::task::ActionSignature* m_popping_wl_as; 
      toolbox::task::WorkLoop* m_pushing_wl;
      toolbox::task::ActionSignature* m_pushing_wl_as;

    private:
      // workloop method
      bool popping(toolbox::task::WorkLoop* wl);
      bool pushing(toolbox::task::WorkLoop* wl);
    private:
      SmartDataSource(const SmartDataSource&);
      SmartDataSource& operator=(const SmartDataSource&);
      
    }; //end class SmartDataSource
  }// end namespace mypackage
}// end namespace bril
#endif
