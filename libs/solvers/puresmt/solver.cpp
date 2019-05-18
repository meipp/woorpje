#include <sstream>
#include <iostream>

#include <numeric>

#include "words/exceptions.hpp"
#include "words/words.hpp"
#include "words/linconstraint.hpp"
#include "smt/smtsolvers.hpp"
#include "solver.hpp"

namespace Words {
  namespace Solvers {
	namespace PureSMT {
	  void retriveSubstitution (Words::SMT::Solver& s, Words::Options& opt, Words::Substitution& sub) {
		for (auto v :opt.context.getVariableAlphabet ()) {
		  Words::Word w;
		  auto wb = opt.context.makeWordBuilder (w);
		  s.evaluate (v,*wb);
		  sub.insert (std::make_pair (v,w));
		}
	  }
	  
	  
	  ::Words::Solvers::Result Solver::Solve (Words::Options& opt,::Words::Solvers::MessageRelay& relay)   {
		relay.pushMessage ("Encode to SMT");
		auto smtsolver = Words::SMT::makeSolver ();
		buildEquationSystem (*smtsolver,opt);
		
		auto res = smtsolver->solve ();
		switch (res) {
		case Words::SMT::SolverResult::Satis:
		  retriveSubstitution (*smtsolver,opt,sub);
		  return ::Words::Solvers::Result::HasSolution;
		  break;
		case Words::SMT::SolverResult::NSatis:
		  return ::Words::Solvers::Result::DefinitelyNoSolution;
		  break;
		case Words::SMT::SolverResult::Unknown:
		  return ::Words::Solvers::Result::NoIdea;
		  break;
		}
		return ::Words::Solvers::Result::DefinitelyNoSolution;
	  }
	}
	
  }
}
