#include <memory>
#include "words/words.hpp"

namespace Words {
  namespace SMT {
	enum class SMTSolver {
				Z3
	};

	enum class SolverResult {
							 Satis,
							 NSatis,
							 Unknown
	};
	
	
	class Solver {
	public:
	  virtual ~Solver () {}
	  virtual SolverResult solve () {return SolverResult::Unknown;}
	  virtual void addVariable (Words::Variable* ) {}
	  virtual void addTerminal (Words::Terminal* ) {}
	  virtual void addConstraint (const Constraints::Constraint& ) {}
	  virtual void addEquation (Words::Equation& ) {}

	  template<class iterator>
	  void addEquations (iterator begin, iterator end) {
		for  (; begin != end;++begin ) {
		  addEquation (*begin);
		}
	  }

	  template<class iterator>
	  void addConstraints (iterator begin, iterator end) {
		for  (; begin != end;++begin ) {
		  addConsraint (**begin);
		}
	  }
	};
	using Solver_ptr = std::unique_ptr<Solver>;
	
	
	void setSMTSolver (SMTSolver);
	Solver_ptr makeSolver (); 
	
  }
}
