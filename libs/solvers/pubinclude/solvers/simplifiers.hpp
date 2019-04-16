#ifndef _SIMPLIFIER__
#define _SIMPLIFIER__

#include "words/words.hpp"
//#include "words/algorithms.hpp"

namespace Words {
  namespace Solvers {
	enum class Simplified {
	  ReducedSatis,
	  JustReduced,
	  ReducedNsatis
	};

	template<class T>
	class Simplifier {
	public:
	  ~Simplifier () {}
	  
	  virtual Simplified solverReduce  (T&) {return Simplified::JustReduced;}
	};

	using EquationSimplifier = Simplifier<Words::Equation>;
	using EquationSystemSimplifier = Simplifier<Words::Options>;

	
	
	template<class First, class Second,class T = Words::Equation>
	class SequenceSimplifier  {
	public:
	  Simplified solverReduce  (T& eq) {
		auto res = first.solverReduce (eq);
		if (res == Simplified::ReducedSatis || res==Simplified::ReducedNsatis)
		  return res;
		else
		  return second.solverReduce (eq);
	  }
	private:
	  First first;
	  Second second;
	};

	template<class Sub>
	class RunAllEq : public EquationSystemSimplifier {
	public:
	  Simplified solverReduce  (Words::Options& opt) {
		std::vector<Equation> eqs;
		for (auto& eq :  opt.equations) {
		  auto res = sub.solverReduce (eq);
		  switch (res) {
		  case Simplified::JustReduced:
			eqs.push_back (eq);
			break;
		  case Simplified::ReducedNsatis:
			return Simplified::ReducedNsatis;
			break;
		  case Simplified::ReducedSatis:
			break;
		  }
		  
		}
		opt.equations.clear();
		opt.equations = eqs;
		return Simplified::JustReduced;
	  }
	private:
	  Sub sub;
	};
	
	class PrefixReducer : public EquationSimplifier{
	public:
	  virtual Simplified solverReduce  (Words::Equation& eq) {
		std::vector<Words::IEntry*> left;
		std::vector<Words::IEntry*> right;
		bool match = true;
		auto llit = eq.lhs.begin();
		auto llend = eq.lhs.end();
		auto rrit = eq.rhs.begin();
		auto rrend = eq.rhs.end();
		for (; llit != llend && rrit!=rrend; ++llit,++rrit) {
		  if (*llit != *rrit) {
			break;
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


	// Strips equal pre-/sufficies of an equation
	class PreSuffixReducer : public EquationSimplifier{
	public:
	  virtual Simplified solverReduce  (Words::Equation& eq) {
		std::vector<Words::IEntry*> left;
		std::vector<Words::IEntry*> right;
		bool match = true;
		auto llit = eq.lhs.begin();
		auto llend = eq.lhs.end();
		auto rrit = eq.rhs.begin();
		auto rrend = eq.rhs.end();
		for (; llit != llend && rrit!=rrend; ++llit,++rrit) {
		  if (*llit != *rrit) {
			break;
		  }
		}

		if (llit != llend && rrit!=rrend){
			rrend--;llend--;
			for (; llit != llend && rrit!=rrend; --llend,--rrend) {
			  if (*llend != *rrend) {
				break;
			  }
			}
			llend++;rrend++;
		}

		for (;llit != llend; ++llit)
		  left.push_back (*llit);
		for (;rrit != rrend; ++rrit)
		  right.push_back (*rrit);

		eq.lhs = std::move(left);
		eq.rhs = std::move(right);


		/// TEST
		/*
		Words::Algorithms::ParikhMatrix p_pm;
		Words::Algorithms::ParikhMatrix s_pm;
		Words::Algorithms::calculateParikhMatrices(eq.lhs,p_pm,s_pm);

		for (auto v : p_pm){
			for (auto x : v){
				std::cout << x.first << " : " << x.second << std::endl;
			}
			std::cout << "-----" << std::endl;
		}*/




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








	
	
	using CoreSimplifier = SequenceSimplifier<
	  RunAllEq<PreSuffixReducer> ,
	  Simplifier<Words::Options>,
	  Words::Options
	  >;
	
	
  }
}

#endif
