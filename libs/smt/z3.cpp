#include <z3.h>
#include <memory>
#include <unordered_map>
#include "smt/smtsolvers.hpp"

namespace Words {
  namespace Z3 {
	class Z3Solver : public Words::SMT::Solver {
	public:
	  Z3Solver () {
		cfg = Z3_mk_config ();
		context = Z3_mk_context(cfg);
		
	  }
	  
	  virtual ~Z3Solver () {}
	  virtual Words::SMT::SolverResult solve () {return Words::SMT::SolverResult::Unknown;}
	  virtual void addVariable (Words::Variable* ){
	  }
	  virtual void addTerminal (Words::Terminal* ) {}
	  virtual void addConstraint (const Constraints::Constraint& ) {}
	  virtual void addEquation (Words::Equation& ) {}

	private:
	  Z3_context context;
	  Z3_config cfg;
	  std::unordered_map<Words::IVariable*,Z3_ast> asts;
	  Z3_sort strsort;
	};

	Words::SMT::Solver_ptr makeZ3Solver () {
	  return std::make_unique<Z3Solver> ();
	}
  }
}
