#ifndef _SAT_SOLVER__
#define _SAT_SOLVER__

#include <sstream>

#include "words/words.hpp"
#include "words/constraints.hpp"
#include "solvers/solvers.hpp"
#include "solvers/timing.hpp"

namespace Words {
  namespace Solvers {
	namespace SatEncoding {
	  template<bool encoding>
	  class Solver : public ::Words::Solvers::Solver {
	  public:
		Solver (size_t bound) : bound(bound) {} 
		Result Solve (Words::Options&,Words::Solvers::MessageRelay&) override;
		//Should only be called if Result returned HasSolution
		void getResults (Words::Solvers::ResultGatherer& r) override;
		virtual void enableDiagnosticOutput () override {
		  diagnostic= true;
		}
	  private:
		bool handleConstraint (Words::Constraints::Constraint&, ::Words::Solvers::MessageRelay&,Words::IEntry* = nullptr);
		Words::Substitution sub;
		std::stringstream diagStr;
		bool diagnostic = false;
		Words::Solvers::Timing::Keeper timekeep;
		size_t bound;
	  };
	}

	template<>
	Solver_ptr makeSolver<Types::SatEncoding,size_t> (size_t bound ) {return std::make_unique<SatEncoding::Solver<true>> (bound);}

	template<>
	Solver_ptr makeSolver<Types::SatEncodingOld,size_t> (size_t bound) {return std::make_unique<SatEncoding::Solver<false>> (bound);}
	
	
  }
}

#endif 
