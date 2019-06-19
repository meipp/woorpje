#include "solvers/solvers.hpp"
#include "heuristics.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {
	  
	  SMTHeuristic_ptr ptr = nullptr;
	  
	  SMTHeuristic& getSMTHeuristic () {
		return *ptr;
	  }

	  void selectVariableTerminalRatio (double d) {ptr = std::make_unique<VariableTerminalRatio> (d) ;}
	  void selectWaitingListReached (size_t a) { ptr = std::make_unique<WaitingListLimitReached> (a) ;}
	  
	}
  }
}
