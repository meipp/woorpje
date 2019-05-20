#include "smt/smtsolvers.hpp"

namespace Words {
  namespace Z3 {
	Words::SMT::Solver_ptr makeZ3Solver ();
  }
  
  namespace SMT {
	
	SMTSolver msolve = SMTSolver::Z3;
	
	void setSMTSolver (SMTSolver i) {msolve =  i;}
	Solver_ptr makeSolver () {
	  switch (msolve) {
#ifdef ENABLEZ3
	  case SMTSolver::Z3:
		return Words::Z3::makeZ3Solver ();
#endif
	  default:
		throw SMTSolverUnavailable ();
	  }

	}
  }
}
