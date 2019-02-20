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
		Result Solve (Words::Options&,Words::Solvers::MessageRelay&) override;
		//Should only be called if Result returned HasSolution
		void getResults (Words::Solvers::ResultGatherer& r) override;
		virtual void enableDiagnosticOutput () {
		  diagnostic= true;
		}
	  private:
		bool handleConstraint (Words::Constraints::Constraint&, ::Words::Solvers::MessageRelay&,Words::IEntry* = nullptr);
		Words::Substitution sub;
		std::stringstream diagStr;
		bool diagnostic = false;
		Words::Solvers::Timing::Keeper timekeep;
	  };
	}
  }
}

#endif 
