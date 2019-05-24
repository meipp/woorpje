#include <sstream>
#include <iostream>

#include <numeric>

#include "words/exceptions.hpp"
#include "words/words.hpp"
#include "words/linconstraint.hpp"
#include "solver.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {

	  
	  ::Words::Solvers::Result Solver::Solve (Words::Options& opt,::Words::Solvers::MessageRelay& relay)   {
		relay.pushMessage ("Levis Algorithm");
		return ::Words::Solvers::Result::NoIdea;
	  }
	}
	
  }
}
