#ifndef _SAT_SOLVER__
#define _SAT_SOLVER__

#include <sstream>

#include "words/words.hpp"
#include "words/constraints.hpp"
#include "solvers/solvers.hpp"
#include "solvers/timing.hpp"

namespace Words {
  namespace Solvers {
	namespace PureSMT {

	  
	  class Solver : public ::Words::Solvers::Solver {
	  public:
		Solver ()  {} 
		Result Solve (Words::Options&,Words::Solvers::MessageRelay&) override;
		//Should only be called if Result returned HasSolution
		void getResults (Words::Solvers::ResultGatherer& r) override {
		  r.setSubstitution (sub);
		}
		
	  private:
		Words::Substitution sub;
	  };
	}

	template<>
	Solver_ptr makeSolver<Types::PureSMT> ( ) {return std::make_unique<PureSMT::Solver> ();}
	
	
	
  }
}

#endif 
