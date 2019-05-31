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
	size_t defaultTimeout = 0;
	
	void setDefaultTimeout (size_t t) {
	  defaultTimeout = t;
	}
	
	void setSMTSolver (SMTSolver i) {msolve =  i;}
	Solver_ptr makeSolver () {
	  Solver_ptr solver;
	  switch (msolve) {
#ifdef ENABLEZ3
	  case SMTSolver::Z3:
		solver = Words::Z3::makeZ3Solver ();
		break;
#endif
#ifdef ENABLECVC4
	  case SMTSolver::CVC4:
		solver = Words::CVC4::makeCVC4Solver ();
		break;
#endif
	  default:
		throw SMTSolverUnavailable ();
	  }
	  if (defaultTimeout ) {
		solver->setTimeout (defaultTimeout);
	  }
	  return solver;

	}
  }
}
