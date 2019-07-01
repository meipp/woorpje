#include "smt/smtsolvers.hpp"

namespace Words {
  namespace Z3 {
	Words::SMT::Solver_ptr makeZ3Solver ();
	Words::SMT::IntSolver_ptr makeZ3IntSolver ();
	
	void setStr3 ();
  }

  namespace CVC4 {
	Words::SMT::Solver_ptr makeCVC4Solver ();
	Words::SMT::IntSolver_ptr makeCVC4IntSolver ();
  }
  
  namespace SMT {
	
	SMTSolver msolve = SMTSolver::Z3;
	size_t defaultTimeout = 0;
	
	void setDefaultTimeout (size_t t) {
	  defaultTimeout = t;
	}
	
	void setSMTSolver (SMTSolver i) {
#ifdef ENABLEZ3
	  if (i ==  SMTSolver::Z3Str3) {
		Words::Z3::setStr3 ();
	  }
#endif
	  msolve =  i;
	}
	
	Solver_ptr makeSolver () {
	  Solver_ptr solver;
	  switch (msolve) {
#ifdef ENABLEZ3
	  case SMTSolver::Z3:
	  case SMTSolver::Z3Str3:
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

	IntSolver_ptr makeIntSolver () {
	  IntSolver_ptr solver;
	  switch (msolve) {
#ifdef ENABLEZ3
	  case SMTSolver::Z3:
	  case SMTSolver::Z3Str3:
		solver = Words::Z3::makeZ3IntSolver ();
		break;
#endif
#ifdef ENABLECVC4
	  case SMTSolver::CVC4:
		solver = Words::CVC4::makeCVC4IntSolver ();
		break;
#endif
	  }
	  return solver;
	}
  }
}
