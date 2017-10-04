#ifndef _bril_mypackage_Events_h_
#define _bril_mypackage_Events_h_
#include "toolbox/Event.h"
#include "xcept/Exception.h"
#include <string>

namespace bril{
    namespace mypackage{

      class QueueAlmostFullEvent : public toolbox::Event{
      public:
      QueueAlmostFullEvent():toolbox::Event("urn:bril-mypackage-event:QueueAlmostFull",0){}
	~QueueAlmostFullEvent(){}
      };

      class QueueAlmostEmptyEvent : public toolbox::Event{
      public:
      QueueAlmostEmptyEvent():toolbox::Event("urn:bril-mypackage-event:QueueAlmostEmpty",0){}
	~QueueAlmostEmptyEvent(){}
      };

      class QueueEmptyEvent : public toolbox::Event{
      public:
      QueueEmptyEvent():toolbox::Event("urn:bril-mypackage-event:QueueEmpty",0){}
	~QueueEmptyEvent(){}
      };

      class QueueOverflowEvent : public toolbox::Event{
      public:
	xcept::Exception exp_;
      QueueOverflowEvent(const xcept::Exception& exp):toolbox::Event("urn:bril-mypackage-event:QueueOverflow",0),exp_(exp){}
	~QueueOverflowEvent(){}	
      };
      
      class LumiSectionChangedEvent : public toolbox::Event{
      public:
      LumiSectionChangedEvent():toolbox::Event("urn:bril-mypackage-event:LumiSectionChanged",0){}
	~LumiSectionChangedEvent(){}
      };

      class NB4ChangedEvent : public toolbox::Event{
      public:
      NB4ChangedEvent():toolbox::Event("urn:bril-mypackage-event:NB4Changed",0){}
	~NB4ChangedEvent(){}
      };

    }
}
#endif
