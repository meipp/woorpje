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
	  
	  
	  
	  ::Words::Solvers::Result Solver::Solve (Words::Options& opt,::Words::Solvers::MessageRelay& relay)   {
		relay.pushMessage (Words::Solvers::Formatter (("Encode to SMT")).str());
		if (opt.hasIneqquality ())
		  return ::Words::Solvers::Result::NoIdea;
		
		auto smtsolver = Words::SMT::makeSolver ();
		buildEquationSystem (*smtsolver,opt);
		relay.pushMessage ((Words::Solvers::Formatter (("Using: %1%")) % smtsolver->getVersionString ()).str());
		auto res = smtsolver->solve ();
		switch (res) {
		case Words::SMT::SolverResult::Satis:
		  Words::SMT::retriveSubstitution (*smtsolver,opt,sub);
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
