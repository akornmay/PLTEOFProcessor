#ifndef _bril_mypackage_HelloWorld_h_
#define _bril_mypackage_HelloWorld_h_
#include <string>
#include "xdaq/Application.h"
#include "xgi/framework/UIManager.h"
#include "xgi/framework/Method.h"
#include "xgi/exception/Exception.h"


namespace bril{
  namespace mypackage{

    class HelloWorld : public xdaq::Application, public xgi::framework::UIManager{
    public:
      //macro helper for object creation
      XDAQ_INSTANTIATOR(); 
      // constructor
      HelloWorld (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
      // destructor
      ~HelloWorld ();
      // xgi(web) callback
      void Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);
     
    private:
      std::string m_mymessage;
      int m_clickcounter;
    private:
      HelloWorld(const HelloWorld&);
      HelloWorld& operator=(const HelloWorld&);
      
    }; //end class HelloWorld
  }// end namespace mypackage
}// end namespace bril
#endif
