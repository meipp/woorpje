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
    auto e = s.equations[0]; // grab the first equations; maybe add some cool heuristics here...
    Words::Substitution subs;
    First::runRule (e,subs);

    if (subs.size() == 0)
        RuleSequencer<Handler,RuleSequence...>::runRules(h,s);
    else {
        std::vector<Words::Equation> newEquations;
        for(auto ee : s.equations){
            for(auto x : subs){ // should only contain a single substitution at this point
                ee.lhs.substitudeVariable(x.first,x.second);
                ee.rhs.substitudeVariable(x.first,x.second);
                newEquations.push_back(ee);
            }
        }
        auto snew = s.copy();
        snew->equations = newEquations;

        //std::cout << typeid(First).name() << std::endl;

        if (!h.handle (s,snew,subs)) {
          RuleSequencer<Handler,RuleSequence...>::runRules(h,s);
        }
    }
  }
};

template<class Handler,class First>
struct RuleSequencer<Handler,First> {
  static void runRules (Handler& h,const Words::Options& s) {
    auto e = s.equations[0]; // grab the first equations; maybe add some cool heuristics here...
    Words::Substitution subs;
    First::runRule (e,subs);

    if (subs.size() == 0)
        return;

    std::vector<Words::Equation> newEquations;
    for(auto ee : s.equations){
        for(auto x : subs){ // should only contain a single substitution at this point
            ee.lhs.substitudeVariable(x.first,x.second);
            ee.rhs.substitudeVariable(x.first,x.second);
            newEquations.push_back(ee);
        }
    }

    //std::cout << typeid(First).name() << std::endl;

    auto snew = s.copy();
    snew->equations = newEquations;
    h.handle (s,snew,subs);
  }
  
};


struct DummyRule {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
    return;
  }
};

// Actual rules
// Xw = Yw' /\ X = YX --> w = Xw'
struct PrefixReasoningLeftHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.begin());
      Words::IEntry* rhsFirst = *(e.rhs.begin());
      if(!(lhsFirst->isVariable() && rhsFirst->isVariable()))
          return;
      Words::Word w ({rhsFirst,lhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(lhsFirst,w) );
  }
};

// Xw = Yw' /\ Y = XY --> Xw = w'
struct PrefixReasoningRightHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.begin());
      Words::IEntry* rhsFirst = *(e.rhs.begin());
      if(!(lhsFirst->isVariable() && rhsFirst->isVariable()))
          return;
      Words::Word w ({lhsFirst,rhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(rhsFirst,w) );
  }
};

// Xw = Yw' /\ Y = X --> w = w'
struct PrefixReasoningEqual {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.begin());
      Words::IEntry* rhsFirst = *(e.rhs.begin());
      if(!(lhsFirst->isVariable() && rhsFirst->isVariable()))
          return;
      Words::Word w ({lhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(rhsFirst,w) );
  }
};

// wX = w'Y /\ X = XY --> wX = w'
struct SuffixReasoningLeftHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.rbegin());
      Words::IEntry* rhsFirst = *(e.rhs.rbegin());
      if(!(lhsFirst->isVariable() && rhsFirst->isVariable()))
          return;
      Words::Word w ({lhsFirst,rhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(lhsFirst,w) );
  }
};

// wX = w'Y /\ Y = YX --> w = w'Y
struct SuffixReasoningRightHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.rbegin());
      Words::IEntry* rhsFirst = *(e.rhs.rbegin());
      if(!(lhsFirst->isVariable() && rhsFirst->isVariable()))
          return;
      Words::Word w ({rhsFirst,lhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(rhsFirst,w) );
  }
};

// wX = w'Y /\ Y = X --> w = w'
struct SuffixReasoningEqual {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.rbegin());
      Words::IEntry* rhsFirst = *(e.rhs.rbegin());
      if(!(lhsFirst->isVariable() && rhsFirst->isVariable()))
          return;
      Words::Word w ({lhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(rhsFirst,w) );
  }
};

// Xw = aw' /\ X = 1 --> w = aw'
struct PrefixEmptyWordLeftHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.begin());
      Words::IEntry* rhsFirst = *(e.rhs.begin());
      if(!(lhsFirst->isVariable() && rhsFirst->isTerminal()))
          return;
      Words::Word w ({});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(lhsFirst,w) );
  }
};

// aw = Xw' /\ X = 1 --> aw = w'
struct PrefixEmptyWordRightHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.begin());
      Words::IEntry* rhsFirst = *(e.rhs.begin());
      if(!(rhsFirst->isVariable() && lhsFirst->isTerminal()))
          return;
      Words::Word w ({});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(rhsFirst,w) );
  }
};

// Xw = aw' /\ X = aX --> Xw = w'
struct PrefixLetterLeftHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.begin());
      Words::IEntry* rhsFirst = *(e.rhs.begin());
      if(!(lhsFirst->isVariable() && rhsFirst->isTerminal()))
          return;
      Words::Word w ({rhsFirst,lhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(lhsFirst,w) );
  }
};

// aw = Xw' /\ X = aX --> w = Xw'
struct PrefixLetterRightHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.begin());
      Words::IEntry* rhsFirst = *(e.rhs.begin());
      if(!(rhsFirst->isVariable() && lhsFirst->isTerminal()))
          return;
      Words::Word w ({lhsFirst,rhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(rhsFirst,w) );
  }
};

//
// wX = w'a /\ X = 1 --> w = w'a
struct SuffixEmptyWordLeftHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.rbegin());
      Words::IEntry* rhsFirst = *(e.rhs.rbegin());
      if(!(lhsFirst->isVariable() && rhsFirst->isTerminal()))
          return;
      Words::Word w ({});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(lhsFirst,w) );
  }
};

// wa = w'X /\ X = 1 --> wa = w'
struct SuffixEmptyWordRightHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.rbegin());
      Words::IEntry* rhsFirst = *(e.rhs.rbegin());
      if(!(rhsFirst->isVariable() && lhsFirst->isTerminal()))
          return;
      Words::Word w ({});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(rhsFirst,w) );
  }
};

// wX = w'a /\ X = Xa --> wX = w'
struct SuffixLetterLeftHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.rbegin());
      Words::IEntry* rhsFirst = *(e.rhs.rbegin());
      if(!(lhsFirst->isVariable() && rhsFirst->isTerminal()))
          return;
      Words::Word w ({lhsFirst,rhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(lhsFirst,w) );
  }
};

// wa = w'X /\ X = Xa --> w = w'X
struct SuffixLetterRightHandSide {
  static void runRule (const Words::Equation& e, Words::Substitution& sub) {
      if (e.lhs.characters() < 1 || e.rhs.characters() < 1)
          return;
      Words::IEntry* lhsFirst = *(e.lhs.rbegin());
      Words::IEntry* rhsFirst = *(e.rhs.rbegin());
      if(!(rhsFirst->isVariable() && lhsFirst->isTerminal()))
          return;
      Words::Word w ({rhsFirst,lhsFirst});
      sub.insert (std::pair<Words::IEntry*,Words::Word>(rhsFirst,w) );
  }
};




#endif
