#include "smt/smtsolvers.hpp"

namespace Words {
  namespace Z3 {
	Words::SMT::Solver_ptr makeZ3Solver ();
  }

  namespace CVC4 {
	Words::SMT::Solver_ptr makeCVC4Solver ();
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
#ifdef ENABLECVC4
	  case SMTSolver::CVC4:
		return Words::CVC4::makeCVC4Solver ();
#endif
	  default:
		throw SMTSolverUnavailable ();
	  }

	}
  }
}
