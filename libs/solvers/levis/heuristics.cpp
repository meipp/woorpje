#include "solvers/solvers.hpp"
#include "heuristics.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {
	  
	  SMTHeuristic_ptr ptr = std::make_unique<SMTHeuristic> ();
	  
	  SMTHeuristic& getSMTHeuristic () {
		return *ptr;
	  }

	  void selectVariableTerminalRatio (double d) {ptr = std::make_unique<VariableTerminalRatio> (d) ;}
	  void selectWaitingListReached (size_t a) { ptr = std::make_unique<WaitingListLimitReached> (a) ;}
	  void selectCalculateTotalEquationSystemSize (double d) { ptr = std::make_unique<CalculateTotalEquationSystemSize> (d) ;}
	  void selectEquationLengthExceeded (size_t a) { ptr = std::make_unique<EquationLengthExceeded> (a) ;}
	  void selectNone () { ptr = std::make_unique<NoSMT> () ;}
	  
	}
  }
}
