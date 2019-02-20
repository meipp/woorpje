#include <sstream>
#include <iostream>

#include "words/exceptions.hpp"
#include "words/words.hpp"
#include "words/linconstraint.hpp"
#include "solver.hpp"

void setupSolverMain (std::vector<std::string>&, std::vector<std::string>&, size_t global);

void addLinearConstraint (const std::string& lhs, const std::string& rhs);

template<bool>
::Words::Solvers::Result runSolver (const bool squareAuto, const Words::Context&,Words::Substitution&,Words::Solvers::Timing::Keeper&,std::ostream*);
  
namespace Words {
  namespace Solvers {
	namespace SatEncoding {
	  template<bool encoding>
	  ::Words::Solvers::Result Solver<encoding>::Solve (Words::Options& opt,::Words::Solvers::MessageRelay& relay)   {
		relay.pushMessage ("SatSolver Ready");
		if (!opt.context.conformsToConventions ())  {
		  relay.pushMessage ("Context does not conform to Upper/Lower-case convention");
		  return ::Words::Solvers::Result::NoIdea;
		}

		Words::IEntry* entry = nullptr;
	
		std::stringstream str;
		std::vector<std::string> lhs;
		std::vector<std::string> rhs;
		
		for (auto& eq : opt.equations) {
		  str.str("");
		  for (auto e : eq.lhs) {
			if (!entry && e->isTerminal ())
			  entry = e;
			str << e->getRepr ();
		  }
		  lhs.push_back (str.str());
		  str.str("");
		  for (auto e : eq.rhs) {
			if (!entry && e->isTerminal ())
			  entry = e;
			str << e->getRepr ();
		  }
		  rhs.push_back (str.str());
		}

		
		for (auto& constraint : opt.constraints) {
		  if (!handleConstraint (*constraint,relay,entry))
			return ::Words::Solvers::Result::NoIdea;
		}
		
		setupSolverMain (lhs,rhs,opt.varMaxPadding);
		return runSolver<encoding> (false,opt.context,sub,timekeep,(diagnostic ? &diagStr : nullptr));
		
	  }
	  
	  //Should only be called if Result returned HasSolution
	  template<bool encoding>
	  void Solver<encoding>::getResults (Words::Solvers::ResultGatherer& r)  {
		r.setSubstitution (sub);
		r.timingInfo (timekeep);
		if(diagnostic)
		  r.diagnosticString (diagStr.str());
	  }
	  
	  template<bool encoding>
	  bool Solver<encoding>::handleConstraint (Words::Constraints::Constraint& c, ::Words::Solvers::MessageRelay& relay,Words::IEntry* e ) {
		std::cerr << "Handle " << c << std::endl; 
		Words::Constraints::LinearConstraint* lc = c.getLinconstraint ();
		if (lc) {
		  if (!e) {
			relay.pushMessage ("Missing Dummy Variable");
			return false;
		  }
		  std::stringstream lhs;
		  std::stringstream rhs;

		  for (auto& vm : *lc) {
			if (vm.number <= 0) {
			  relay.pushMessage ("Variable multiplicity must be strictly greater than zero");
			  return false;
			}
			char c = vm.entry->getRepr ();
			for (int64_t i = 0; i < vm.number; i++) {
			  lhs << c;
			}
		  }
		  auto rhs_n = lc->getRHS ();
		  if (rhs_n <= 0) {
			relay.pushMessage ("RHS of lineary constraint must be strictly larger than zero");
			return false;
		  }
		  for (int64_t i = 0; i < rhs_n; i++) {
			rhs << e->getRepr();
		  }
		  addLinearConstraint (lhs.str(),rhs.str());
		  return true;
		}
		else {
		  relay.pushMessage ("Only Lineary Constraints are supported");
		  return false;
		}


	  
	  }
	}
	
	
	template<>
	Solver_ptr makeSolver<Types::SatEncoding> () {return std::make_unique<SatEncoding::Solver<true>> ();}

	template<>
	Solver_ptr makeSolver<Types::SatEncodingOld> () {return std::make_unique<SatEncoding::Solver<false>> ();}
  }
}
