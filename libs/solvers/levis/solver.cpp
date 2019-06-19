#include <sstream>
#include <iostream>

#include <numeric>

#include "words/algorithms.hpp"
#include "words/exceptions.hpp"
#include "words/words.hpp"
#include "words/linconstraint.hpp"
#include "passed.hpp"
#include "graph.hpp"
#include "solver.hpp"
#include "rules.hpp"
#include "heuristics.hpp"
#include "solvers/simplifiers.hpp"
#include "smt/smtsolvers.hpp"
#include "words/algorithms.hpp"

#include <sstream>
#include <iostream>

#include <numeric>

namespace Words {
  namespace Solvers {
	namespace Levis {

	  class Handler {
	  public:
		Handler (PassedWaiting& w, Graph& g,Words::Substitution& s) : waiting(w),graph(g),subs(s) {}
		
        // criteria for external solver calls
        //

        // trigger the solver with a timeout given via parameter every time the
        // waiting list exceeds a bound
        /*bool waitingListLimitExceeded(std::size_t wSize, std::size_t bound=35){
            return wSize > bound;
			}*/

        // sets the solver timeout based on items in the waiting list
        /*bool waitingListLimitExceededScaleTimeout(std::size_t wSize, std::size_t& timeout, std::size_t bound=35){
            if(!waitingListLimitExceeded(wSize,bound))
                return false;
            timeout = (wSize-bound)*10;
            return true;
			}*/

        /*bool variableLimitReached(std::shared_ptr<Words::Options>& to, std::size_t bound=3){
            return to->context.getVariableAlphabet().size() < bound;
			}*/

        // some equations (maximumEquations amount) exceeded a length bound
        /*bool equationLengthExceeded(std::shared_ptr<Words::Options>& to, std::size_t maximumLength=50, std::size_t maximumEquations=0){
            //Words::Algorithms::ParikhImage p_lhs;
            auto eqBegin = to->equations.begin();
            auto eqEnd = to->equations.end();
            for(auto it = eqBegin; it != eqEnd; ++it){
                if (it->lhs.characters() + it->rhs.characters() > maximumLength){
                    if (!maximumEquations)
                        return true;
                    maximumEquations--;
                }
            }
            return false;
			}*/

        size_t calculateTotalEquationSystemSize(const Words::Options& o){
            size_t eqsSize = 0;
            auto eqBegin = o.equations.begin();
            auto eqEnd = o.equations.end();
            for(auto it = eqBegin; it != eqEnd; ++it){
                eqsSize = eqsSize + it->lhs.characters() + it->rhs.characters();
            }
            return eqsSize;
        }

        // Equation grows too quickly...
        bool equationLengthGrowthExceeded(const Words::Options& from, const std::shared_ptr<Words::Options>& to, double scale=1.25){
            std::size_t fromSize = calculateTotalEquationSystemSize(from);
            std::size_t toSize = calculateTotalEquationSystemSize(*to);
            return fromSize*scale < toSize;
        }



        // Linear constraints handling
        bool modifyLinearConstraints(std::shared_ptr<Words::Options>& opt, const Words::Substitution& sub){
		  if (opt->constraints.size() == 0 || sub.size() == 0)
			return true;

		  
		  std::unique_ptr<Words::Constraints::LinearConstraintBuilder> builder = nullptr;
		  std::vector<Constraints::Constraint_ptr> newConstraints;
		  
		  std::cout << sub << std::endl;
		  for (auto x : sub){
			auto cBegin = opt->constraints.begin();
			auto cEnd   = opt->constraints.end();
			for (auto it = cBegin; it!=cEnd; ++it){
			  if (!(*it)->isLinear()){
				newConstraints.push_back((*it));
				continue;
			  }
			  
			  builder = Words::Constraints::makeLinConstraintBuilder (Words::Constraints::Cmp::LEq);
			  auto lhsBegin = ((*it)->getLinconstraint())->begin();
			  auto lhsEnd = ((*it)->getLinconstraint())->end();
			  Constraints::Constraint_ptr cstr;
			  
			  // r(X) = \epsilon
			  if (x.second.characters() == 0){
				builder->addRHS((*it)->getLinconstraint()->getRHS());
				for (auto lhsIt = lhsBegin; lhsIt != lhsEnd; ++lhsIt){
				  if ((*lhsIt).entry != x.first){
					builder->addLHS((*lhsIt).entry,(*lhsIt).number);
				  }
				}
				
				cstr = builder->makeConstraint();
				if(cstr->lhsEmpty()){
				  if(cstr->getLinconstraint()->getRHS() < 0){
                                return false;
				  }
				} else {
				  newConstraints.push_back(cstr);
				}
				continue;
			  }
			  
			  // we need information about the amout of variables and chars later...
			  size_t variableCount = 0;
			  size_t terminalCount = 0;
			  x.second.sepearteCharacterCount (terminalCount, variableCount);
			  Words::Algorithms::ParikhImage p = Words::Algorithms::calculateParikh(x.second);
			  
			  
			  // do the variable magic
			  std::vector<IEntry*> usedVariables;
			  std::map<IEntry*,int64_t> coefficents;
			  int64_t coefficent = 0;
			  x.second.getVariables (usedVariables);
			  
			  // get the coefficent of the substituions left hand side
			  // rather naive right now...
			  
			  
			  for (auto lhsIt = lhsBegin; lhsIt != lhsEnd; ++lhsIt){
				if ((*lhsIt).entry == x.first){
				  coefficent = (*lhsIt).number;
				  break;
				}
			  }
			  
			  
			  // left hand side not present inside the constraint
			  if (coefficent == 0){
				newConstraints.push_back((*it)->copy());
				continue;
			  }
			  
			  // add the right hand side
			  int64_t rhsSum = (*it)->getLinconstraint()->getRHS()-(coefficent*((int64_t)terminalCount));
			  builder->addRHS(rhsSum);
			  
			  
			  // collect the other coefficents
			  for (auto lhsIt = lhsBegin; lhsIt != lhsEnd; ++lhsIt){
				if(std::find(usedVariables.begin(), usedVariables.end(), (*lhsIt).entry) != usedVariables.end()) {
				  coefficents[(*lhsIt).entry] =  (*lhsIt).number;
				} else if ((*lhsIt).entry != x.first) {
				  builder->addLHS((*lhsIt).entry,(*lhsIt).number);
				}
			  }
			  
			  
			  // fill coefficents for unused variables but occurring in the substitution
			  for (auto y : usedVariables){
				if (!coefficents.count(y))
				  coefficents[y] = 0;
			  }
			  
			  // create left hand side
			  int64_t bSum = 0;
			  for(auto y : coefficents){
				if (y.first != x.first){
				  bSum = p.at(y.first)*coefficent+y.second;
				  if(bSum != 0)
					builder->addLHS(y.first,bSum);
				} else {
				  builder->addLHS(x.first,coefficent);
				}
			  }
			  
			  cstr = builder->makeConstraint();
			  if(cstr->lhsEmpty()){
				std::cout << "NICE" << x.first->getRepr() << x.second << std::endl;
				if(cstr->getLinconstraint()->getRHS() < 0){
				  return false;
				}
			  }
			  else {
				newConstraints.push_back(cstr);
			  }
			}
			if (newConstraints.size() > 0)
			  opt->constraints = newConstraints;
		  }
		  return true;
        }
		
		//returns true if successor generation should stop
		//
		bool handle (const Words::Options& from, std::shared_ptr<Words::Options>& to, const Words::Substitution& sub) {
          //auto beforeSimp = to->copy ();
		  // Simplification
          if(!modifyLinearConstraints(to, sub))
             return false;


  /*        std::cout << "----------------------"<< std::endl;
          std::cout << "Start modification on:" << from << std::endl;
          std::cout << "=====================" <<  std::endl; */
         // modifyLinearConstraints(to, sub);
          auto beforeSimp = to->copy ();
  /*        std::cout << "First modification:" << *to << std::endl;
          std::cout << "Substitution was: " << sub << std::endl;
std::cout << "=====================" <<  std::endl;
*/
		  Words::Substitution simplSub;
          auto res = Words::Solvers::CoreSimplifier::solverReduce (*to,simplSub);


       /*   std::cout << "Second modification:" << *to << std::endl;
          std::cout << "Substitution was: " << simplSub << std::endl;
          std::cout << "----------------------"<< std::endl;
        */
		  if(!modifyLinearConstraints(to, simplSub))
              return false;

		  if (res==Simplified::ReducedNsatis){
			return false;
		  }
		  
          if (waiting.contains(to))
              return false;

		  auto node = graph.getNode (from);			
		  auto simpnode = graph.makeNode (beforeSimp);
		  auto nnode = graph.makeNode (to);

		  graph.addEdge (node,simpnode,sub);

          if (simpnode != nnode)
            graph.addEdge (simpnode,nnode,simplSub);
			
          if (res==Simplified::ReducedSatis) {
			result = Words::Solvers::Result::HasSolution;
            subs = findRootSolution (nnode);
			// rebuild subsitution here!>
			waiting.clear();
			return true;
		  }


          SMTHeuristic_ptr heur = std::make_unique<VariableTerminalRatio> (1.1);
          if (heur->doRunSMTSolver (from,*to,waiting))
            return runSMTSolver (nnode,to,*heur);
			  
		  waiting.insert(to);
			  
			

		  //TODO Check if the equation is
		  // Trivially satisfied,
		  // Trivially unsatisfied, or
		  // If we should call the SMT solver
		  // In either case of satisfied we need som code to reconstruct the substitution
		  // For now we just return false thus apply rules until we reach a fix-point
		  return false;
		}

        bool runSMTSolver (Node* n, const std::shared_ptr<Words::Options>& from, SMTHeuristic& heur) {
		  n->ranSMTSolver = true;
		  auto smtsolver = Words::SMT::makeSolver ();
		  heur.configureSolver (smtsolver);
		  Words::SMT::buildEquationSystem (*smtsolver,*from);
          switch (smtsolver->solve()) {
		  case Words::SMT::SolverResult::Satis: {
			std::shared_ptr<Words::Options> tt = from->copy ();
			tt->equations.clear();
			
			Words::Substitution finalSolution;
			Words::SMT::retriveSubstitution (*smtsolver,*tt,finalSolution);
			auto nnode = graph.makeNode (tt);
			graph.addEdge (n,nnode,finalSolution);
			subs = findRootSolution (nnode);
			waiting.clear();
			result = Words::Solvers::Result::HasSolution;
			return true;
			break;
		  }
		  case Words::SMT::SolverResult::Unknown:
			waiting.insert (from);
			break;
		  case Words::SMT::SolverResult::NSatis:
			break;
		  } 
		  return false;
		}
		
        auto getResult () const {
            return result;}
		
	  private:
		PassedWaiting& waiting;
		Graph& graph;
		Words::Substitution& subs;
		Words::Solvers::Result result = Words::Solvers::Result::NoIdea;
	  };
	  
	  ::Words::Solvers::Result Solver::Solve (Words::Options& opt,::Words::Solvers::MessageRelay& relay)   {
		relay.pushMessage ("Levis Algorithm");
		PassedWaiting waiting;
		Graph graph;
		Handler handler (waiting,graph,sub);

#ifdef ENABLEGRAPH
		GuaranteeOutput go ("Levis",graph);
#endif
		
        // We need the substituion of the first simplifier run. This is just a quick hack...
        // Start THE SOLVER without simplify flag
        Words::Substitution simplSub;
		auto first = opt.copy();
		auto insert = opt.copy ();
        auto res = Words::Solvers::CoreSimplifier::solverReduce (*insert,simplSub);
		handler.modifyLinearConstraints(insert, simplSub);
		auto inode = graph.makeNode (insert);
		

        if (res==Simplified::ReducedNsatis){
          return Words::Solvers::Result::DefinitelyNoSolution;
        } else if (res==Simplified::ReducedSatis) {
		  auto fnode = graph.makeNode (first); 
		  graph.addEdge (fnode,inode,simplSub);
		  sub = findRootSolution (inode);
          return Words::Solvers::Result::HasSolution;
        }
        
        //auto insert = opt.copy ();
		waiting.insert (insert);
		
		while (waiting.size()) {
          auto cur = waiting.pullElement ();
          RuleSequencer<Handler,PrefixReasoningLeftHandSide,PrefixReasoningRightHandSide,PrefixReasoningEqual,PrefixEmptyWordLeftHandSide,PrefixEmptyWordRightHandSide,PrefixLetterLeftHandSide,PrefixLetterRightHandSide,SuffixReasoningLeftHandSide,SuffixReasoningRightHandSide,SuffixReasoningEqual,SuffixEmptyWordLeftHandSide,SuffixEmptyWordRightHandSide,SuffixLetterLeftHandSide,SuffixLetterRightHandSide>::runRules (handler,*cur);
          relay.progressMessage ((Words::Solvers::Formatter ("Passed: %1%, Waiting: %2%") % waiting.passedsize() % waiting.size()).str());
		}

        



        if (handler.getResult() == Words::Solvers::Result::NoIdea ){
            return Words::Solvers::Result::DefinitelyNoSolution;
        }


        return handler.getResult ();
}





	}
	
  }
}
