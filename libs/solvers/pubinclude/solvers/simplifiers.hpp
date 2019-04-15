#ifndef _SIMPLIFIER__
#define _SIMPLIFIER__

#include "words/words.hpp"

namespace Words {
  namespace Solvers {
	enum class Simplified {
	  ReducedSatis,
	  JustReduced,
	  ReducedNsatis
	};
	
	class Simplifier {
	public:
	  ~Simplifier () {}
	  
	  virtual Simplified solverReduceEquation  (Words::Equation&) {return Simplified::JustReduced;}
	};

	template<class First, class Second>
	class SequenceSimplifier : Simplifier {
	public:
	  Simplified solverReduceEquation  (Words::Equation& eq) {
		auto res = first.solverReduceEquation (eq);
		if (res == Simplified::ReducedSatis || res==Simplified::ReducedNsatis)
		  return res;
		else
		  return second.solverReduceEquation (eq);
	  }
	private:
	  First first;
	  Second second;
	};
	
	class PrefixReducer : public Simplifier{
	public:
	  virtual Simplified solverReduceEquation  (Words::Equation& eq) {
		std::vector<Words::IEntry*> left;
		std::vector<Words::IEntry*> right;
		bool match = true;
		auto llit = eq.lhs.begin();
		auto llend = eq.lhs.end();
		auto rrit = eq.rhs.begin();
		auto rrend = eq.rhs.end();
		for (; llit != llend && rrit!=rrend; ++llit,++rrit) {
		  if (*llit != *rrit) {			break;
		  }
		}
		for (;llit != llend; ++llit)
		  left.push_back (*llit);
		for (;rrit != rrend; ++rrit)
		  right.push_back (*rrit);
		
		eq.lhs = std::move(left);
		eq.rhs = std::move(right);
		if (eq.lhs.characters () == 0 &&
			eq.rhs.characters () == 0)
		  return Simplified::ReducedSatis;
		else if (eq.lhs.characters () == 0 ||
				 eq.rhs.characters () == 0)
		  return Simplified::ReducedNsatis;
		else
		  return Simplified::JustReduced;
		
	  }
	};

	using CoreSimplifier = SequenceSimplifier<PrefixReducer,Simplifier> ;
	
	
  }
}

#endif
