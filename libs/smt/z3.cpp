#include <z3.h>
#include <memory>
#include <sstream>
#include <unordered_map>
#include "words/exceptions.hpp"
#include "smt/smtsolvers.hpp"

namespace Words {
  namespace Z3 {
	class Z3Solver : public Words::SMT::Solver {
	public:
	  Z3Solver () {
		cfg = Z3_mk_config ();
		context = Z3_mk_context(cfg);
		solver = Z3_mk_solver(context);
		strsort = Z3_mk_string_sort (context);
	  }
	  
	  virtual ~Z3Solver () {}

	  virtual Words::SMT::SolverResult solve () {
		
		switch (Z3_solver_check (context,solver)) {
		case Z3_L_TRUE:
		  model = Z3_solver_get_model (context,solver);
		  return Words::SMT::SolverResult::Satis;
		  break;
		case Z3_L_FALSE:
		  return Words::SMT::SolverResult::Satis;
		  break;
		case Z3_L_UNDEF:
		default:
		  return Words::SMT::SolverResult::Unknown;
		}
	  }
	  
	  virtual void addVariable (Words::Variable* v){
		std::stringstream str;
		str << v->getRepr ();
		auto symb = Z3_mk_string_symbol (context,str.str().c_str());
		auto ast = Z3_mk_const (context,symb,strsort);
		asts.insert (std::make_pair (v,ast));
	  }
	  
	  virtual void addTerminal (Words::Terminal* ) {}

	  virtual void addConstraint (const Constraints::Constraint& ) {
		throw Words::ConstraintUnsupported ();
	  }

	  virtual void evaluate (Words::Variable* v, Words::WordBuilder& wb) {
		Z3_ast ast;
		Z3_model_eval (context,model,asts.at(v),true,&ast);
		auto str = Z3_get_string (context,ast);
		while (*str != '\0') {
		  wb << *str;
		  str++;
		}
		
	  }
	  
	  
	  virtual void addEquation (const Words::Equation& eq) {
		auto left = buildWordAst (eq.lhs);
		auto right = buildWordAst (eq.rhs);
		auto eqe = Z3_mk_eq (context,left,right);
		Z3_solver_assert (context,solver,eqe);
	  }

	  Z3_ast buildWordAst (const Words::Word& w) {
		std::vector<Z3_ast> asts;
		
		std::stringstream str;
		for (auto i : w) {
		  if (i->isVariable ()) {
			if (str.str().size ()) {
			  auto seq = Z3_mk_string (context,str.str().c_str());
			  asts.push_back (seq);
			  str.str("");
			}
			asts.push_back ( this->asts.at(i));
		  }
		  else {
			str << i->getRepr ();
		  }
		}
		
		if (str.str ().size()) {
		  auto seq = Z3_mk_string (context,str.str().c_str());
		  asts.push_back (seq);
		}

		if (asts.size() > 1) 
		  return Z3_mk_seq_concat (context,asts.size(),asts.data());				  else
		  return asts[0];
		  
		
	  }

	private:
	Z3_context context;
	Z3_config cfg;
	std::unordered_map<Words::IEntry*,Z3_ast> asts;
	Z3_sort strsort;
	Z3_solver solver;
	Z3_model model;
  };
  
	Words::SMT::Solver_ptr makeZ3Solver () {
	  return std::make_unique<Z3Solver> ();
	}
  }
}
