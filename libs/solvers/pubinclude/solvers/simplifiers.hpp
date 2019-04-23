#ifndef _SIMPLIFIER__
#define _SIMPLIFIER__

#include "words/words.hpp"
#include "words/algorithms.hpp"

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

	template<class First, class T = Words::Equation>
	class InnerSequenceSimplifier  {
	protected:
		Simplified innerSolverReduce  (T& eq) {
				return first.solverReduce (eq);
		  }
		private:
		First first;
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


	class ConstSequenceMismatch : public EquationSimplifier{
	public:
	  virtual Simplified solverReduce  (Words::Equation& eq) {
		Words::Word constSide;
		Words::Word variableSide;
		std::vector<Words::Word> constSequences;
		if(eq.lhs.noVariableWord() && !eq.rhs.noVariableWord()){
			constSide = eq.lhs;
			variableSide = eq.rhs;
		} else if(eq.rhs.noVariableWord() && !eq.lhs.noVariableWord()){
			constSide = eq.rhs;
			variableSide = eq.lhs;
		} else
			return Simplified::JustReduced;

		constSequences = variableSide.getConstSequences();
		for (Word w : constSequences){
			if(!constSide.isFactor(w)){
				return Simplified::ReducedNsatis;
			}
		}
		return Simplified::JustReduced;
	  }
	};



	template<class T>
	class SubstitutionReasoning : public InnerSequenceSimplifier<T,Words::Equation> {
	public:
	  Simplified solverReduce  (Words::Options& opt) {
		std::vector<Equation> eqs;
		std::unordered_map<IEntry*,Word> substitution;

		// Assumes that equation are minimized due to prefix and suffix
		for (auto& eq :  opt.equations) {
			if(eq.rhs.noVariableWord() && eq.lhs.characters() == 1 && eq.lhs.get(0)->isVariable()){
				auto it = substitution.find(eq.lhs.get(0));
				if (it != substitution.end()){
					if(it->second != eq.rhs){
						return Simplified::ReducedNsatis;
					}
				}
				substitution[eq.lhs.get(0)] = eq.rhs;
			}
			if(eq.lhs.noVariableWord() && eq.rhs.characters() == 1 && eq.rhs.get(0)->isVariable()){
				auto it = substitution.find(eq.rhs.get(0));
				if (it != substitution.end()){
					if(it->second != eq.lhs){
						return Simplified::ReducedNsatis;
					}
				}
				substitution[eq.rhs.get(0)] = eq.lhs;
			}
		}

		if (substitution.size() > 0){
			bool skipEquation = false;
			auto it = opt.equations.begin();
			auto end = opt.equations.end();

			for (;it != end; ++it) {
				Word& lhs = it->lhs;
				Word& rhs = it->rhs;
				for (auto s : substitution){
					IEntry* var = s.first;
					Word w = s.second;

					bool rSubs = rhs.substitudeVariable(var,w);
					bool lSubs = lhs.substitudeVariable(var,w);

					if (rSubs || lSubs){
						if (lhs != rhs) {
							auto res = this->innerSolverReduce(*it);
							if (res == Simplified::ReducedNsatis){
								return res;
							}

							if (lhs.noVariableWord() && rhs.noVariableWord()){
								  if (lhs != rhs){
									  return Simplified::ReducedNsatis;
								  }
							  }
							it->lhs = lhs;
							it->rhs = rhs;
						} else {
							skipEquation = true;
						}
					}
				}
				if (!skipEquation){
					eqs.push_back(*it);
				}
				skipEquation = false;
			}
			opt.equations = eqs;
	  	  }

		return Simplified::JustReduced;
	  }
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

	// Uses the length arguments for unsat check
	class ParikhMatrixMismatch : public EquationSimplifier{
	public:
	  virtual Simplified solverReduce  (Words::Equation& eq) {
		  Words::Algorithms::ParikhMatrix lhs_p_pm;
		  Words::Algorithms::ParikhMatrix lhs_s_pm;
		  Words::Algorithms::ParikhMatrix rhs_p_pm;
		  Words::Algorithms::ParikhMatrix rhs_s_pm;
		  Words::Algorithms::calculateParikhMatrices(eq.lhs,lhs_p_pm,lhs_s_pm);
		  Words::Algorithms::calculateParikhMatrices(eq.rhs,rhs_p_pm,rhs_s_pm);
		  bool processPrefix = true;
		  bool processSuffix = false;
		  bool terminalsAlignPrefix = true;
		  bool terminalsAlignSuffix = true;
		  int rSize = eq.rhs.characters();
		  int lSize = eq.lhs.characters();
		  int minSize = std::min(rSize,lSize);
		  int sri = 0;
		  int sli = 0;
		  auto terminalAlphabet  = eq.ctxt->getTerminalAlphabet();
		  auto variableAlphabet = eq.ctxt->getVariableAlphabet();

		  for (int i = 1; i < minSize; i++){
			sri = (rSize-1)-i;
			sli = (lSize-1)-i;

			Words::Algorithms::ParikhImage lhs_p_pi = lhs_p_pm[i];
			Words::Algorithms::ParikhImage rhs_p_pi = rhs_p_pm[i];
			Words::Algorithms::ParikhImage lhs_s_pi = lhs_s_pm[i];
			Words::Algorithms::ParikhImage rhs_s_pi = rhs_s_pm[i];

			// Process variables
			for (auto x : variableAlphabet){
				if(processPrefix){
					if(lhs_p_pi[x] != rhs_p_pi[x]){
						processPrefix = false;
					}
				}
				if(processSuffix){
					if(lhs_s_pi[x] != rhs_s_pi[x]){
						processSuffix = false;
					}
				}
				if(!processPrefix && !processSuffix){
					break;
				}
			}
			// Process terminals
			for (auto x : terminalAlphabet){
				if(processPrefix){
					if(lhs_p_pi[x] != rhs_p_pi[x]){
						terminalsAlignPrefix = false;
					}
				}
				if(processSuffix){
					if(lhs_s_pi[x] != rhs_s_pi[x]){
						terminalsAlignSuffix = false;
					}
				}
				if(!processPrefix && !processSuffix){
					break;
				}
			}

			if ((processPrefix && !terminalsAlignPrefix) || (processSuffix && !terminalsAlignSuffix)){
				return Simplified::ReducedNsatis;
			}

			processPrefix = true;
			processSuffix = false;
			terminalsAlignPrefix = true;
			terminalsAlignSuffix = true;
		}
		return Simplified::JustReduced;;
	  }
	};

	// Checks for pre/suffix mismatching terminals
	class CharacterMismatchDetection : public EquationSimplifier{
	public:
	  virtual Simplified solverReduce  (Words::Equation& eq) {
		bool processPrefix = true;
		bool processSuffix = true;
		int rSize = eq.rhs.characters();
		int lSize = eq.lhs.characters();
		int minSize = std::min(rSize,lSize);

		for (int i = 0; i < minSize; i++){
			IEntry* r = eq.rhs.get(i);
			IEntry* l = eq.lhs.get(i);
			IEntry* rr = eq.rhs.get((rSize-1)-i);
			IEntry* ll = eq.lhs.get((lSize-1)-i);
			if(processPrefix){
				if (r->isTerminal() && l->isTerminal()  && l != r){
					return Simplified::ReducedNsatis;
				} else if (l != r){
					processPrefix = false;
				}
			}
			if(processSuffix){
				if (rr->isTerminal() && ll->isTerminal() && ll != rr){
					return Simplified::ReducedNsatis;
				} else if (ll != rr){
					processSuffix = false;
				}
			}
			if(!processPrefix && !processSuffix){
				return Simplified::JustReduced;;
			}
		}
		return Simplified::JustReduced;;
	  }
	};
	
	using CoreSimplifier = SequenceSimplifier<
			RunAllEq<SequenceSimplifier<PreSuffixReducer, SequenceSimplifier<CharacterMismatchDetection,ParikhMatrixMismatch>>>,
			//RunAllEq<PreSuffixReducer>,
			SubstitutionReasoning<SequenceSimplifier<PreSuffixReducer,SequenceSimplifier<ConstSequenceMismatch,SequenceSimplifier<CharacterMismatchDetection,ParikhMatrixMismatch>>>>,
			Words::Options>;




	/*using CoreSimplifier = SequenceSimplifier<
	  RunAllEq<reSuffixReducer> ,
	  SequenceSimplifier<
	  	  RunAllEq<CharacterMismatchDetection>,
		  RunAllEq<ParikhMatrixMismatch>,
	  	  Words::Options
	  	  >,
	  Words::Options
	  >;
		*/
	
  }
}

#endif
