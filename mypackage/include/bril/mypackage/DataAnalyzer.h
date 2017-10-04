#ifndef _bril_mypackage_DataAnalyzer_h_
#define _bril_mypackage_DataAnalyzer_h_
#include <string>
#include <vector>
#include <map>
#include "xdaq/Application.h"
#include "xgi/framework/UIManager.h"
#include "xgi/framework/Method.h"
#include "xgi/exception/Exception.h"
#include "xdata/InfoSpace.h"
#include "xdata/ActionListener.h"
#include "xdata/String.h"
#include "toolbox/ActionListener.h"
#include "toolbox/EventDispatcher.h"
#include "toolbox/task/WorkLoop.h"
#include "eventing/api/Member.h"
#include "b2in/nub/exception/Exception.h"
#include "toolbox/BSem.h"

namespace bril{
  namespace mypackage{

    /**
       A xdaq application subscribes/collects beam, and bcm1fagghist topics from bril eventing
       1. analyze the background channels and publish the result bcm1fbkg to the same eventing immediately (on NB4 frequency)
       2. analyze the luminosity channels and publish the result bcm1flumi to the same eventing on LumiSectionChanged event  

       Set NOSTORE flag of the outgoing topics to "1" all the time except for beam status="STABLE BEAMS","ADJUST","FLATTOP","SQUEEZE"

       Note: This is a demonstration, it does not match any real bcm1f algorithm
     */
    class DataAnalyzer : public xdaq::Application, 
                                       public xgi::framework::UIManager, 
                                       public xdata::ActionListener, 
                                       public toolbox::ActionListener, 
                                       public toolbox::EventDispatcher,
                                       public eventing::api::Member
{
    public:
      //macro helper for xdaq object creation
      XDAQ_INSTANTIATOR(); 
      // constructor
      DataAnalyzer (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
      // destructor
      ~DataAnalyzer ();
      // my xgi(web) callback
      void Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);
      // xdata::Event callback
      void actionPerformed( xdata::Event& );
      // toolbox::Event callback
      void actionPerformed( toolbox::Event& );     
      // my eventing message callback
      void onMessage( toolbox::mem::Reference*, xdata::Properties& ) throw ( b2in::nub::exception::Exception );

      // workloop publishing to eventing
      toolbox::task::WorkLoop* m_publishlumi_wl;
      toolbox::task::ActionSignature* m_publishlumi_wl_as;
      toolbox::task::WorkLoop* m_publishbkg_wl;
      toolbox::task::ActionSignature* m_publishbkg_wl_as;
 
      bool publishlumi( toolbox::task::WorkLoop* wl );
      bool publishbkg( toolbox::task::WorkLoop* wl );

    private:
      //configuration parameters
      xdata::String m_bus;
      //data cache
      std::string m_beamstatus;
      std::map< unsigned int, std::vector<unsigned short> > m_mylumihist; // channelid, bunchhist
      std::map< unsigned int, std::vector<unsigned short> > m_mybkghist; //channelid, bunchhist
      unsigned int m_fillnum;
      unsigned int m_runnum;
      unsigned int m_lsnum;
      unsigned int m_nbnum;
      unsigned int m_timestampsec;
      unsigned int m_timestampmsec;
      toolbox::BSem m_applock;

      //application memory pool for eventing messages
      bool m_canpublish;
      //   toolbox::mem::MemoryPoolFactory *m_poolFactory;
      toolbox::mem::Pool* m_memPool;
     
    private:
      DataAnalyzer(const DataAnalyzer&);
      DataAnalyzer& operator=(const DataAnalyzer&);
      
    }; //end class DataAnalyzer
  }// end namespace mypackage
}// end namespace bril
#endif
