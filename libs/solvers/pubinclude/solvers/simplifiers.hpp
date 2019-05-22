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
	  
	  static Simplified solverReduce  (T&, Words::Substitution& s) {return Simplified::JustReduced;}
	};
	

	using EquationSimplifier = Simplifier<Words::Equation, Words::Substitution>;
	using EquationSystemSimplifier = Simplifier<Words::Options, Words::Substitution>;

	template<class T,class... Ts>
	class SequenceSimplifier2;

	template<class T,class First>
	class SequenceSimplifier2<T,First> {
	public:
	  static Simplified solverReduce  (T& eq,Substitution& s) {
		return First::solverReduce (eq,s);
	  }
	};
	
	template<class T,class First,class... Ts>
	class SequenceSimplifier2<T,First,Ts...> {
	public:
	  
	  static Simplified solverReduce  (T& eq,Substitution& s) {
		auto res = First::solverReduce (eq,s);
		if (res == Simplified::ReducedSatis || res==Simplified::ReducedNsatis)
		  return res;
		else {
		  s.clear ();
		  return SequenceSimplifier2<T,Ts...>::solverReduce (eq,s);
		}
	  }

	};

	template<class First, class T = Words::Equation>
	class InnerSequenceSimplifier  {
	protected:
	  static Simplified innerSolverReduce  (T& eq) {
		static Words::Substitution s;
		s.clear ();
		return First::solverReduce (eq,s);
	  }
	};

	template<class Sub>
	class RunAllEq : public EquationSystemSimplifier {
	public:
	  static Simplified solverReduce  (Words::Options& opt, Substitution& s) {
		std::vector<Equation> eqs;
		for (auto& eq :  opt.equations) {
		  s.clear();
		  auto res = Sub::solverReduce (eq,s);
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
		if (opt.equations.size ())
		  return Simplified::JustReduced;
		return Simplified::ReducedSatis;
	  }

	};


	class PrefixReducer {
	public:
	  static Simplified solverReduce  (Words::Equation& eq, Substitution& s) {
		auto& lhs = eq.lhs;
		auto& rhs = eq.rhs;
		while (lhs.ebegin () != lhs.eend () &&
			   rhs.ebegin () !=rhs.eend ()) {
		Word::entry_iterator lfirst = eq.lhs.ebegin();
		Word::entry_iterator rfirst = eq.rhs.ebegin();
		if (*lfirst == *rfirst) {
		  lhs.erase_entry (lfirst);
		  rhs.erase_entry (rfirst);
		}
		else if ((*lfirst)->isSequence () && (*rfirst)->isSequence ()) {
		  Words::Sequence* ll = (*lfirst)->getSequence ();
		  Words::Sequence* rr = (*rfirst)->getSequence ();
		  if (*ll < *rr) {
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
		else
		  break;
		}
		if (lhs.ebegin () == lhs.eend () &&
			rhs.ebegin () == rhs.eend ()) {
		  return Simplified::ReducedSatis;
		}
	  
		return Simplified::JustReduced;
	  }
	};
	  
	class SuffixReducer {
	public:
	  static Simplified solverReduce  (Words::Equation& eq, Substitution& s) {
		auto& lhs = eq.lhs;
		auto& rhs = eq.rhs;
		while (lhs.ebegin () != lhs.eend () &&
			   rhs.ebegin () !=rhs.eend ()) {
		  
		  auto lrfirst = lhs.rebegin();
		  auto rrfirst = rhs.rebegin();
		  if (*lrfirst == *rrfirst) {
			eq.lhs.erase_entry (lrfirst);
			eq.rhs.erase_entry (rrfirst);
		  }
		  else if ((*lrfirst)->isSequence () && (*rrfirst)->isSequence ()) {
			Words::Sequence* ll = (*lrfirst)->getSequence ();
			Words::Sequence* rr = (*rrfirst)->getSequence ();
			if (ll->isSuffixOf (*rr)) {
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
		  else {
			break;
		  }
		  
		}
		if (lhs.ebegin () == lhs.eend () &&
			rhs.ebegin () == rhs.eend ()) {
		  return Simplified::ReducedSatis;
		}
		return Simplified::JustReduced;
	  }
	};

	
	// Strips equal pre-/sufficies of an equation	
	using PreSuffixReducer = SequenceSimplifier2<Words::Equation,PrefixReducer,SuffixReducer>;
	
	class ConstSequenceMismatch : public EquationSimplifier{
	public:
	  static Simplified solverReduce  (Words::Equation& eq,Substitution& s) {
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

	class ConstSequenceFolding {
	public:
	  static Simplified solverReduce  (Words::Equation& eq,Substitution& s) {
		foldWord (eq.lhs,eq);
		foldWord (eq.rhs,eq);
		return Simplified::JustReduced;
	  };

	  static void foldWord (Words::Word& lhs,Words::Equation& eq) {
		std::vector<Words::IEntry*> nword;
		Words::Context::SeqInput nseq;
		
		Words::Word::entry_iterator begin = lhs.ebegin();
		Words::Word::entry_iterator end = lhs.eend();
		for (auto it = begin; it != end ; ++it) {
		  if (!(*it)->isSequence()) {
			if (nseq.size()) {
			  nword.push_back (eq.ctxt->addSequence (nseq));
			  nseq.clear ();
			}
			nword.push_back (*it);
			  
		  }
		  else {
			auto seq = (*it)->getSequence();
			std::copy (seq->begin (),seq->end (),std::back_inserter (nseq));
		  }
		}
		
		if (nseq.size ()) {
		  nword.push_back (eq.ctxt->addSequence (nseq));
		}
		lhs = std::move(nword);
	  }
	};
	

	template<class Inner>
	class SubstitutionReasoningNew {
	public:
	  static Simplified replaceVarInEquation (Words::Equation& eq,  Words::IEntry* var, Words::Word& repl) {
		Substitution subs;
		eq.lhs.substitudeVariable (var,repl);
		eq.rhs.substitudeVariable (var,repl);
		return Inner::solverReduce (eq,subs);
	  }

	  static Simplified solverReduce  (Words::Options& opt, Substitution& substitution) {
		std::vector<Words::Equation>::iterator it = opt.equations.begin();
		std::vector<Words::Equation>::iterator end = opt.equations.end();
		while (it != end) {
		  IEntry* variable = nullptr;
		  Word* subsWord = nullptr;
		  if (it->lhs.entries () == 1 && (*it->lhs.begin ())->isVariable()) {
			variable = (*it->lhs.begin ());
			subsWord = &it->rhs;
		  }

		  if (it->rhs.entries () == 1 && (*it->rhs.begin ())->isVariable()) {
			variable = (*it->rhs.begin ());
			subsWord = &it->lhs;
		  }

		  if (variable) {
			std::vector<Words::Equation> eqs;
			assert(subsWord);
			for (auto& mapit : substitution) {
			  mapit.second.substitudeVariable (variable,*subsWord);
			}
			
			substitution.insert(std::make_pair (variable,*subsWord));
			
			
			for (auto iit = opt.equations.begin(); iit != opt.equations.end();++iit) {
			  if (it == iit)
				continue;
			  auto res = replaceVarInEquation (*iit,variable,*subsWord); 
			  if ( res == Simplified::ReducedNsatis)  {
				return Simplified::ReducedNsatis;
			  }
			  else if ( res == Simplified::JustReduced) {
				eqs.push_back (*iit);
			  }
			}
			opt.equations = eqs;
			it = opt.equations.begin();
			end = opt.equations.end();
		  }
		  else 
			++it;
		}
		
		if (opt.equations.size ()){
		  for (auto x : substitution){
			Substitution dummy;
			Words::Equation eq;
			eq.lhs = { x.first };
			eq.rhs = x.second;
			eq.ctxt = &opt.context;
			ConstSequenceFolding::solverReduce (eq,dummy);
			opt.equations.push_back(eq);
		  }
		  

		  
		  return Simplified::JustReduced;
		  
		}
		else {
		  return Simplified::ReducedSatis;
		}
		
	  }
	  
	  
	};

	/*template<class Inner>
	class SubstitutionReasoning {// : public InnerSequenceSimplifier<T,Words::Equation> {
	public:
	  static Simplified innerSolverReduce (Words::Equation& eq) {
		static Words::Substitution s;
		s.clear ();
		return Inner::solverReduce (eq,s);
	  }

	  static Simplified solverReduce  (Words::Options& opt, Substitution& substitution) {
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
							  auto res = innerSolverReduce(*it);
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
	  };*/


	// Uses the length arguments for unsat check
	class ParikhMatrixMismatch : public EquationSimplifier{
	public:
	  static Simplified solverReduce  (Words::Equation& eq,Substitution&) {
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


	using FoldPreSuf = SequenceSimplifier2<Words::Equation,ConstSequenceFolding,PrefixReducer,SuffixReducer>;
	using CoreSimplifier = SequenceSimplifier2<Words::Options,RunAllEq<FoldPreSuf>,
											   SubstitutionReasoningNew<FoldPreSuf>
											   >;
	
	//using CoreSimplifier = RunAllEq<PrefixReducer>;
	//using CoreSimplifier = RunAllEq<SequenceSimplifier<ConstSequenceMismatch,
	//PreSuffixReducer
	//>
	//								>;
									
	
	
  }
}

#endif
