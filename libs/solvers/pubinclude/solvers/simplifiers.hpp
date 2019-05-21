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

	template<class T,class Substitution = Words::Substitution>
	class Simplifier {
	public:
	  ~Simplifier () {}
	  
	  virtual Simplified solverReduce  (T&, Words::Substitution& s) {return Simplified::JustReduced;}
	};

	using EquationSimplifier = Simplifier<Words::Equation, Words::Substitution>;
	using EquationSystemSimplifier = Simplifier<Words::Options, Words::Substitution>;

	
	
	template<class First, class Second,class T = Words::Equation>
	class SequenceSimplifier  {
	public:
	  Simplified solverReduce  (T& eq,Substitution& s) {
		auto res = first.solverReduce (eq,s);
		if (res == Simplified::ReducedSatis || res==Simplified::ReducedNsatis)
		  return res;
		else {
		  s.clear ();
		  return second.solverReduce (eq,s);
		}
	  }
	private:
	  First first;
	  Second second;
	};

	template<class First, class T = Words::Equation>
	class InnerSequenceSimplifier  {
	protected:
	  Simplified innerSolverReduce  (T& eq) {
		static Words::Substitution s;
		s.clear ();
		return first.solverReduce (eq,s);
	  }
	private:
	  First first;
	};

	template<class Sub>
	class RunAllEq : public EquationSystemSimplifier {
	public:
	  Simplified solverReduce  (Words::Options& opt, Substitution& s) {
		std::vector<Equation> eqs;
		for (auto& eq :  opt.equations) {
		  s.clear();
		  auto res = sub.solverReduce (eq,s);
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
	  virtual Simplified solverReduce  (Words::Equation& eq, Substitution& s) {
		Word::entry_iterator lfirst = eq.lhs.ebegin();
		Word::entry_iterator rfirst = eq.rhs.ebegin();
		if ((*lfirst)->isSequence () && (*rfirst)->isSequence ()) {
		  Words::Sequence* ll = (*lfirst)->getSequence ();
		  Words::Sequence* rr = (*rfirst)->getSequence ();
		  if (*ll == *rr) {
 			eq.lhs.erase_entry (lfirst);
			eq.rhs.erase_entry (rfirst);
		  }
		  
		  else if (*ll < *rr) {
			auto seqdiff = *rr - *ll;
			auto seq = eq.ctxt->addSequence (seqdiff);
			eq.lhs.erase_entry (lfirst);
			eq.rhs.replace_entry (rfirst,seq);
			
		  }

		  else if (*rr < *ll) {
			auto seqdiff = *ll - *rr;
			auto seq = eq.ctxt->addSequence (seqdiff);
			eq.rhs.erase_entry (rfirst);
			eq.lhs.replace_entry (lfirst,seq);
			
		  }
		  else 
			return Simplified::ReducedNsatis;
		}
		
		
		auto lrfirst = eq.lhs.rebegin();
		auto rrfirst = eq.rhs.rebegin();
		if ((*lrfirst)->isSequence () && (*rrfirst)->isSequence ()) {
		  
		  Words::Sequence* ll = (*lrfirst)->getSequence ();
		  Words::Sequence* rr = (*rrfirst)->getSequence ();
		  if (*ll == *rr) {
			eq.lhs.erase_entry (lrfirst);
			eq.rhs.erase_entry (rrfirst);
		  }
		  
		  else if (ll->isSuffixOf (*rr)) {
			auto seqdiff = rr->chopTail (*ll);
			auto seq = eq.ctxt->addSequence (seqdiff);
			eq.lhs.erase_entry (lrfirst);
			eq.rhs.replace_entry (rrfirst,seq);
		  }
		  
		  else if (rr->isSuffixOf (*ll)) {
			auto seqdiff = ll->chopTail (*rr);
			auto seq = eq.ctxt->addSequence (seqdiff);
			eq.rhs.erase_entry (rrfirst);
			eq.lhs.replace_entry (lrfirst,seq);
		  }
		  else 
			return Simplified::ReducedNsatis;
		}
		return Simplified::JustReduced;
	  }
	};

	
	
	class ConstSequenceMismatch : public EquationSimplifier{
	public:
	  virtual Simplified solverReduce  (Words::Equation& eq,Substitution& s) {
		Words::Word* constSide;
		Words::Word* variableSide;
		if(eq.lhs.noVariableWord() && !eq.rhs.noVariableWord()){
		  constSide = &eq.lhs;
		  variableSide = &eq.rhs;
		} else if(eq.rhs.noVariableWord() && !eq.lhs.noVariableWord()){
		  constSide = &eq.rhs;
		  variableSide = &eq.lhs;
		} else
		  return Simplified::JustReduced;

		std::vector<Words::Sequence*> consts;
		variableSide->getSequences (consts);
		assert ((*constSide->ebegin())->isSequence ());
		Words::Sequence* constSeq = (*constSide->ebegin())->getSequence (); 
		for (auto seq : consts) {
		  if (!seq->isFactorOf (*constSeq)) {
			return Simplified::ReducedNsatis;
		  }
		}
		return Simplified::JustReduced;
	  }
	};



	template<class T>
	class SubstitutionReasoning : public InnerSequenceSimplifier<T,Words::Equation> {
	public:
	  Simplified solverReduce  (Words::Options& opt, Substitution& substitution) {
		std::vector<Equation> eqs;
		//std::unordered_map<IEntry*,Word> substitution;

		bool foundNewSubstition = true;

		// quick hack to avoid endless loops
		int maxIterations = 5;
		int iterations = 0;

		while(foundNewSubstition && maxIterations >= iterations){

			foundNewSubstition = false;
			iterations++;

			// Assumes that equation are minimized due to prefix and suffix
			for (auto& eq :  opt.equations) {
				IEntry* variable;
				Word subsWord;
				bool setUp = false;

				if(eq.lhs.characters() == 1 && eq.lhs.get(0)->isVariable()){
					variable = eq.lhs.get(0);
					subsWord = eq.rhs;
					setUp = true;

				}
				if(eq.rhs.characters() == 1 && eq.rhs.get(0)->isVariable()){
					variable = eq.rhs.get(0);
					subsWord = eq.lhs;
					setUp = true;

				}
				if (setUp){
					auto it = substitution.find(variable);
					if (it != substitution.end()){
						if (it->second.noVariableWord() && subsWord.noVariableWord()){
							if(it->second != subsWord){
								return Simplified::ReducedNsatis;
							}
						} else if ((it->second.noVariableWord() && !subsWord.noVariableWord())){
							subsWord = it->second;
						}
					}
					substitution[variable] = subsWord;
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
								//lhs.noVariableWord() && // rhs.noVariableWord() &&
								if ((rhs.characters() == 1 && rhs.get(0)->isVariable()) ||
										(lhs.characters() == 1 && lhs.get(0)->isVariable()))
										foundNewSubstition = true;

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
		}
		if (opt.equations.size ()){
			for (auto x : substitution){
				Words::Equation eq;
				eq.lhs = { x.first };
				eq.rhs = x.second;
				eq.ctxt = &opt.context;
				opt.equations.push_back(eq);
			}



		  return Simplified::JustReduced;

		} else {
		  return Simplified::ReducedSatis;
		}
	  }
	};


	// Uses the length arguments for unsat check
	class ParikhMatrixMismatch : public EquationSimplifier{
	public:
	  virtual Simplified solverReduce  (Words::Equation& eq,Substitution&) {
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
		  size_t rSize = eq.rhs.characters();
		  size_t lSize = eq.lhs.characters();
		  int minSize = std::min(rSize,lSize);
		  int sri = 0;
		  int sli = 0;
		  auto terminalAlphabet  = eq.ctxt->getTerminalAlphabet();
		  auto variableAlphabet = eq.ctxt->getVariableAlphabet();

		  // Quick linear unsat check based on the parik image of the equation
			bool seenTwoVariables = false;
			int sumRhs = 0;
			int coefficentLhs = 0;

			for (auto a : eq.ctxt->getVariableAlphabet()){
				if(coefficentLhs != 0){
						seenTwoVariables = true;
						break; //  saw two variables, we can not do anything at this point
					} else {
						if (rhs_p_pm[rSize-1].count(a) == 1 && lhs_p_pm[lSize-1].count(a) == 1){
							coefficentLhs = (lhs_p_pm[lSize-1][a]-rhs_p_pm[rSize-1][a]);
						} else if (lhs_p_pm[lSize-1].count(a) == 1){
							coefficentLhs = lhs_p_pm[lSize-1][a];
						} else if (rhs_p_pm[rSize-1].count(a) == 1){
							coefficentLhs = -rhs_p_pm[rSize-1][a];
						}
					}
			}
			if (!seenTwoVariables){
				for (auto a : eq.ctxt->getTerminalAlphabet()){
					if (rhs_p_pm[rSize-1].count(a) == 1 && lhs_p_pm[lSize-1].count(a) == 1){
						sumRhs = sumRhs+(lhs_p_pm[lSize-1][a]-rhs_p_pm[rSize-1][a]);
					} else if (lhs_p_pm[lSize-1].count(a) == 1){
						sumRhs = sumRhs+lhs_p_pm[lSize-1][a];
					} else if (rhs_p_pm[rSize-1].count(a) == 1){
						sumRhs = sumRhs-rhs_p_pm[rSize-1][a];
					}
				}

				if (coefficentLhs != 0 && sumRhs % coefficentLhs != 0){
					return Simplified::ReducedNsatis;
				}
			}

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

	//Uncommented. It seems to be unneeded ATM
	/*	class CharacterMismatchDetection : public EquationSimplifier{
	public:
	  virtual Simplified solverReduce  (Words::Equation& eq, Substitution& ) {
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
	  };*/
	
	/*using CoreSimplifier = SequenceSimplifier<
			RunAllEq<SequenceSimplifier<PreSuffixReducer, SequenceSimplifier<CharacterMismatchDetection,ParikhMatrixMismatch>>>,
			//RunAllEq<PreSuffixReducer>,
			SubstitutionReasoning<SequenceSimplifier<PreSuffixReducer,SequenceSimplifier<ConstSequenceMismatch,SequenceSimplifier<CharacterMismatchDetection,ParikhMatrixMismatch>>>>,
			Words::Options>;

	*/

	

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

	
	using CoreSimplifier = RunAllEq<SequenceSimplifier<ConstSequenceMismatch,
													   PreSuffixReducer
													   >
									>;
									
	
	
  }
}

#endif
