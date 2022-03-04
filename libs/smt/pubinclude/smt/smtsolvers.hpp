#ifndef _SMT_SOLVERS__
#define _SMT_SOLVERS__


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
						  Z3Str3,
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
	  virtual void setTimeout (size_t) { throw Words::WordException ("Timeout not implemented");}
	  
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

	  virtual std::string getVersionString () const = 0;
	};
	using Solver_ptr = std::unique_ptr<Solver>;
	
	
	void setSMTSolver (SMTSolver);
	void setDefaultTimeout (size_t);	  
	Solver_ptr makeSolver (); 
	

	inline void retriveSubstitution (Words::SMT::Solver& s, Words::Options& opt, Words::Substitution& sub) {
		for (auto v :opt.context->getVariableAlphabet ()) {
		  Words::Word w;
		  
		  auto wb = opt.context->makeWordBuilder (w);
		  s.evaluate (v,*wb);
		  wb->flush ();
		  sub.insert (std::make_pair (v,w));
		}
	  }
	
	inline void buildEquationSystem (Solver& solver, const Words::Options& opt) {
	  for (auto i : opt.context->getVariableAlphabet ())
		solver.addVariable (i);
	  for (auto i : opt.context->getTerminalAlphabet ())
		solver.addTerminal (i);
	  solver.addEquations (opt.equations.begin(),opt.equations.end());
	  solver.addConstraints (opt.constraints.begin(),opt.constraints.end());
	}

	class IntegerSolver {
	public:
	  virtual ~IntegerSolver () {}
	  virtual void addVariable (const Words::Variable* ) {}
	  virtual void addConstraint (const Constraints::Constraint& ) {}
	  virtual SolverResult solve () {return SolverResult::Unknown;}
	  virtual size_t evaluate (Words::Variable*) = 0;
	  virtual void setTimeout (size_t) { throw Words::WordException ("Timeout not implemented");}
	};

	using IntSolver_ptr = std::unique_ptr<IntegerSolver>;
	IntSolver_ptr makeIntSolver (); 
  }
}

#endif
