#ifndef _RULES
#define _RULES

#include "words/words.hpp"

template<class Handler,class...RuleSequence>
struct RuleSequencer {
  static void runRules (Handler& h,const Words::Options& s) {}
};

template<class Handler,class First, class... RuleSequence>
struct RuleSequencer<Handler,First,RuleSequence...> {
  static void runRules (Handler& h,const Words::Options& s) {
	auto nnew = First::runRule (s);
	if (!h.handle (s,nnew)) {
	  RuleSequencer<Handler,RuleSequence...>::runRule (h);
	}
  }
};

template<class Handler,class First>
struct RuleSequencer<Handler,First> {
  static void runRules (Handler& h,const Words::Options& s) {
	auto nnew = First::runRule (s);
	h.handle (s,nnew);	
  }
  
};

struct DummyRule {
  static std::shared_ptr<Words::Options> runRule (const Words::Options& s) {
	return s.copy ();
  }
};

#endif
