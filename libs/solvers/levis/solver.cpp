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


#include <sstream>
#include <iostream>

#include <numeric>

namespace Words {
  namespace Solvers {
	namespace Levis {

	  class Handler {
	  public:
		Handler (PassedWaiting& w, Graph& g,Words::Substitution& s) : waiting(w),graph(g),subs(s) {}

		//returns true if successor generation should stop
		//
		bool handle (const Words::Options& from, std::shared_ptr<Words::Options>& to, const Words::Substitution& sub) {
		  auto node = graph.getNode (from);


          // Simplification
          Words::Substitution simplSub;
         /* auto res = Words::Solvers::CoreSimplifier::solverReduce (*to,simplSub); //TODO: THIS CALL ISN'T WORKING - WRONG TYPE!!! FIX IT

          if (res==Simplified::ReducedNsatis){
              return false;
          } else if (res==Simplified::ReducedSatis){
              // rebuild subsitution here!
              return true;
          }*/

          // merge substitutions
          // this is stupid, I know ;)
          Words::Substitution newSub;
          for (auto x : simplSub){
              for(auto y : sub){ // should be exactly one substitution due to levis rules
                  if (x.first == y.first){
                     Words::Word w = y.second;
                     w.substitudeVariable(y.first,x.second);
                     newSub.insert(std::pair<Words::IEntry*,Words::Word>(y.first,w));
                  } else if(newSub.count(x.first) != 1){
                    newSub.insert(std::pair<Words::IEntry*,Words::Word>(x.first,x.second));
                  }
              }
          }

          auto nnode = graph.makeNode (to);
          graph.addEdge (node,nnode,newSub);
          waiting.insert(to);
		  
		  //TODO Check if the equation is
		  // Trivially satisfied,
		  // Trivially unsatisfied, or
		  // If we should call the SMT solver
		  // In either case of satisfied we need som code to reconstruct the substitution
		  // For now we just return false thus apply rules until we reach a fix-point
		  return false;
		}

		
	  private:
		PassedWaiting& waiting;
		Graph& graph;
		Words::Substitution& subs;
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

          std::cout << "THIS IS THE CURRENT ELEMENT!" << std::endl;
          for(auto e : (*cur).equations){
              std::cout << e << std::endl;
          }
          std::cout << "DONE!" << std::endl;

          RuleSequencer<Handler,PrefixReasoningLeftHandSide,PrefixReasoningRightHandSide,PrefixReasoningEqual,PrefixEmptyWordLeftHandSide,PrefixEmptyWordRightHandSide,PrefixLetterLeftHandSide,PrefixLetterRightHandSide,SuffixReasoningLeftHandSide,SuffixReasoningRightHandSide,SuffixReasoningEqual,SuffixEmptyWordLeftHandSide,SuffixEmptyWordRightHandSide,SuffixLetterLeftHandSide,SuffixLetterRightHandSide>::runRules (handler,*cur);
		  relay.progressMessage ((Words::Solvers::Formatter ("Passed: %1%, Waiting: %2%") % waiting.passedsize() % waiting.size()).str());
		}
		return ::Words::Solvers::Result::NoIdea;
	  }
	}
	
  }
}
