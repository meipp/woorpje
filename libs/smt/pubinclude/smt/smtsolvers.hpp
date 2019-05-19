#include <memory>
#include "words/exceptions.hpp"
#include "words/words.hpp"

namespace Words {
  namespace SMT {
	class SMTSolverUnavailable : public Words::WordException {
	public:
	  SMTSolverUnavailable () : WordException ("SMT Solver not available") {}
	};
	
	enum class SMTSolver {
						  Z3,
						  CVC4
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
	  virtual void addEquation (const Words::Equation& ) {}
	  virtual void evaluate (Words::Variable*, Words::WordBuilder& wb) = 0;
	  
	  template<class iterator>
	  void addEquations (iterator begin, iterator end) {
		for  (; begin != end;++begin ) {
		  addEquation (*begin);
		}
	  }

	  template<class iterator>
	  void addConstraints (iterator begin, iterator end) {
		for  (; begin != end;++begin ) {
		  addConstraint (**begin);
		}
	  }
	};
	using Solver_ptr = std::unique_ptr<Solver>;
	
	
	void setSMTSolver (SMTSolver);
	Solver_ptr makeSolver (); 

	inline void buildEquationSystem (Solver& solver, const Words::Options& opt) {
	  for (auto i : opt.context.getVariableAlphabet ())
		solver.addVariable (i);
	  solver.addEquations (opt.equations.begin(),opt.equations.end());
	  solver.addConstraints (opt.constraints.begin(),opt.constraints.end());
	}
  }
}
