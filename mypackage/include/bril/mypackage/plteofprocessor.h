#ifndef _bril_mypackage_plteofprocessor_h_
#define _bril_mypackage_plteofprocessor_h_
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "xdaq/Application.h"
#include "xgi/framework/UIManager.h"
#include "xgi/framework/Method.h"
#include "xgi/exception/Exception.h"

#include "xdata/InfoSpace.h"
#include "xdata/ActionListener.h"
#include "xdata/String.h"
#include "xdata/Float.h"
#include "xdata/Boolean.h"
#include "xdata/TimeVal.h"
#include "xdata/Vector.h"
#include "xdata/UnsignedInteger32.h"

#include "eventing/api/Member.h"
#include "b2in/nub/exception/Exception.h"

#include "toolbox/BSem.h"
#include "toolbox/ActionListener.h"
#include "toolbox/EventDispatcher.h"
#include "toolbox/task/WorkLoop.h"
#include "toolbox/task/Timer.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/Reference.h"
#include "toolbox/squeue.h"

#include "bril/mypackage/EventAnalyzer.h"

#include "bril/mypackage/ROC_Efficiency.h"

namespace bril{
  namespace mypackage{
    
    class plteofprocessor : public xdaq::Application, 
      public xgi::framework::UIManager, 
      public xdata::ActionListener, 
      public toolbox::ActionListener, 
      public toolbox::task::TimerListener,
      public toolbox::EventDispatcher,
      public eventing::api::Member
      {
      public:
	//macro helper for xdaq object creation
	XDAQ_INSTANTIATOR(); 
	// constructor
	plteofprocessor (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
	// destructor
	~plteofprocessor ();
	// my xgi(web) callback
	void Default(xgi::Input * in, xgi::Output * out) throw (xgi::exception::Exception);
	// my eventing message callback
	void onMessage( toolbox::mem::Reference*, xdata::Properties& ) throw ( b2in::nub::exception::Exception );
	// xdata::Event callback
	virtual void actionPerformed( xdata::Event& );
	// toolbox::Event callback
	virtual void actionPerformed( toolbox::Event& );     
	virtual void timeExpired(toolbox::task::TimerEvent& e);
 
      private:
	
	//	void subscribeAll();
	void zmqClient();
	//  void makePlots();
	bool publishlumi( toolbox::task::WorkLoop* wl );
	bool publishbkg( toolbox::task::WorkLoop* wl );
	bool writetofile( toolbox::task::WorkLoop* wl );
	bool roc_efficiency( toolbox::task::WorkLoop* wl );
	bool roc_accidentals( toolbox::task::WorkLoop* wl );
	
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

	// temporary stuff to write to file
	std::ofstream tofile;	
	std::string m_filename;

	//application memory pool for eventing messages
	bool m_canpublish;
	
      protected:
	// message exchange
	int m_nibble;
	int m_fill;
	int m_run;
	int m_ls;
	int m_time_orbit;
	int m_orbit;

	std::vector<float> m_ROCEfficiency;
	std::vector<float> m_PLTAccidentals;

	// workloop publishing to eventing
	toolbox::task::WorkLoop* m_publishlumi_wl;
	toolbox::task::ActionSignature* m_publishlumi_wl_as;
	toolbox::task::WorkLoop* m_publishbkg_wl;
	toolbox::task::ActionSignature* m_publishbkg_wl_as;
	// workloop writing pltzero avgrawlumi and avglumi to file;
	toolbox::task::WorkLoop* m_writetofile_wl;
	toolbox::task::ActionSignature* m_writetofile_wl_as;
	// workloop for efficiency 
	toolbox::task::WorkLoop* m_roc_efficiency_wl;
	toolbox::task::ActionSignature* m_roc_efficiency_wl_as;
	// workloop for accidentals 
	toolbox::task::WorkLoop* m_roc_accidentals_wl;
	toolbox::task::ActionSignature* m_roc_accidentals_wl_as;

	void stopTimer();
	void startTimer();
      	toolbox::mem::Pool* m_memPool;
	//	toolbox::mem::MemoryPoolFactory *m_poolFactory;
 	xdata::Float m_avgrawlumi;       //rawbxlumi sum over bx
	xdata::Float m_avglumi;          //bxlumi sum over bx	
	std::string m_timername;
	xdata::String m_workloopHost;
	xdata::String m_slinkHost;
	xdata::String m_gcFile;
	xdata::String m_alFile;
	xdata::String m_trackFile;
	xdata::String m_pixMask;
	xdata::String m_baseDir;
	xdata::Float m_calibVal;
	xdata::Boolean m_MakeLogs;
	xdata::Boolean m_AlwaysFlash;
      private:
	plteofprocessor(const plteofprocessor&);
	plteofprocessor& operator=(const plteofprocessor&);
	
      }; //end class plteofprocessor
  }// end namespace mypackage
}// end namespace bril
#endif
