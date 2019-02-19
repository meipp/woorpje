#include <sstream>

#include "words/exceptions.hpp"
#include "words/words.hpp"
#include "solver.hpp"

void setupSolverMain (std::vector<std::string>&, std::vector<std::string>&, size_t global);

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
		std::vector<std::string> lhs;
		std::vector<std::string> rhs;
		std::stringstream str;
		for (auto& eq : opt.equations) {
		  str.str("");
		  for (auto e : eq.lhs)
			str << e->getRepr ();
		  lhs.push_back (str.str());
		  str.str("");
		  for (auto e : eq.rhs)
			str << e->getRepr ();
		  rhs.push_back (str.str());
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
	}
	
	template<>
	Solver_ptr makeSolver<Types::SatEncoding> () {return std::make_unique<SatEncoding::Solver<true>> ();}

	template<>
	Solver_ptr makeSolver<Types::SatEncodingOld> () {return std::make_unique<SatEncoding::Solver<false>> ();}
  }
}
