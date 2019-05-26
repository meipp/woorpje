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
	Words::Substitution subs;
	auto nnew = First::runRule (s);
	if (!h.handle (s,nnew,subs)) {
	  RuleSequencer<Handler,RuleSequence...>::runRule (h);
	}
  }
};

template<class Handler,class First>
struct RuleSequencer<Handler,First> {
  static void runRules (Handler& h,const Words::Options& s) {
	Words::Substitution subs;
	auto nnew = First::runRule (s,subs);
	h.handle (s,nnew,subs);	
  }
  
};

struct DummyRule {
  static std::shared_ptr<Words::Options> runRule (const Words::Options& s, Words::Substitution& sub) {
	return s.copy ();
  }
};

#endif
