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
        bool waitingListLimitExceeded(std::size_t wSize, std::size_t bound=35){
            return wSize > bound;
        }

        // sets the solver timeout based on items in the waiting list
        bool waitingListLimitExceededScaleTimeout(std::size_t wSize, std::size_t& timeout, std::size_t bound=35){
            if(!waitingListLimitExceeded(wSize,bound))
                return false;
            timeout = (wSize-bound)*10;
            return true;
        }

        bool variableLimitReached(std::shared_ptr<Words::Options>& to, std::size_t bound=3){
            return to->context.getVariableAlphabet().size() < bound;
        }

        // some equations (maximumEquations amount) exceeded a length bound
        bool equationLengthExceeded(std::shared_ptr<Words::Options>& to, std::size_t maximumLength=50, std::size_t maximumEquations=0){
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
        }

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


		//returns true if successor generation should stop
		//
		bool handle (const Words::Options& from, std::shared_ptr<Words::Options>& to, const Words::Substitution& sub) {
		  auto beforeSimp = to->copy ();
		  // Simplification
		  Words::Substitution simplSub;
		  auto res = Words::Solvers::CoreSimplifier::solverReduce (*to,simplSub); 

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

          if (equationLengthGrowthExceeded(from,to))  { //insert criterion for running SMTSolvers
          std::size_t timeout = 0;
          //if (waitingListLimitExceededScaleTimeout(waiting.size(),timeout)){
            return runSMTSolver (nnode,to,timeout);
		  }
		  
		  // merge substitutions
		  /*Words::Substitution newSub = sub;
			for (auto x : simplSub){
			if (newSub.count(x.first))  { // == y.first){
			newSub[x.first].substitudeVariable (x.first,x.second); 
			}
			}*/
			
			  
		  waiting.insert(to);
			  
			

		  //TODO Check if the equation is
		  // Trivially satisfied,
		  // Trivially unsatisfied, or
		  // If we should call the SMT solver
		  // In either case of satisfied we need som code to reconstruct the substitution
		  // For now we just return false thus apply rules until we reach a fix-point
		  return false;
		}

        bool runSMTSolver (Node* n, const std::shared_ptr<Words::Options>& from, std::size_t timeout=0) {
		  n->ranSMTSolver = true;
		  auto smtsolver = Words::SMT::makeSolver ();
		  Words::SMT::buildEquationSystem (*smtsolver,*from);
          if (timeout)
            smtsolver->setTimeout(timeout);
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
		
		auto getResult () const { return result;}
		
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
		
		auto insert = opt.copy ();
		waiting.insert (insert);
		graph.makeNode (insert);

		while (waiting.size()) {
          auto cur = waiting.pullElement ();
          RuleSequencer<Handler,PrefixReasoningLeftHandSide,PrefixReasoningRightHandSide,PrefixReasoningEqual,PrefixEmptyWordLeftHandSide,PrefixEmptyWordRightHandSide,PrefixLetterLeftHandSide,PrefixLetterRightHandSide,SuffixReasoningLeftHandSide,SuffixReasoningRightHandSide,SuffixReasoningEqual,SuffixEmptyWordLeftHandSide,SuffixEmptyWordRightHandSide,SuffixLetterLeftHandSide,SuffixLetterRightHandSide>::runRules (handler,*cur);
          relay.progressMessage ((Words::Solvers::Formatter ("Passed: %1%, Waiting: %2%") % waiting.passedsize() % waiting.size()).str());
		}
#ifdef ENABLEGRAPH
		outputToString ("Levis",graph);
#endif
		return handler.getResult ();
	  }
	}
	
  }
}
