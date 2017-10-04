#include "bril/mypackage/HelloWorld.h" 
#include "cgicc/Cgicc.h"
#include "cgicc/HTMLClasses.h"
#include <iostream>

XDAQ_INSTANTIATOR_IMPL (bril::mypackage::HelloWorld)

namespace bril{
  namespace mypackage{
    HelloWorld::HelloWorld( xdaq::ApplicationStub* s ) throw (xdaq::exception::Exception) : xdaq::Application(s),xgi::framework::UIManager(this){
      xgi::framework::deferredbind(this, this, &bril::mypackage::HelloWorld::Default, "Default");
      m_mymessage = "Hello World";
      m_clickcounter = 0;
      std::cout<<"Load URL "<<getApplicationContext()->getContextDescriptor()->getURL()<<" in browser"<<std::endl;
    }

    HelloWorld::~HelloWorld(){}
    
    void HelloWorld::Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception){
      ++m_clickcounter;
      *out << "<h2 align=\"center\"> Page URL "<<getApplicationContext()->getContextDescriptor()->getURL()<<"</h2>";
      *out << cgicc::br();
      *out << m_mymessage;
      
      std::cout<<m_mymessage<<" are  displayed "<<m_clickcounter<<" times"<<std::endl;
    }

  } // end namespace mypackage
}// end namespace bril
