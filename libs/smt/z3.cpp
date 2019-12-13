#include <z3.h>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <set>
#include "passthrough.hpp"
#include "words/exceptions.hpp"
#include "smt/smtsolvers.hpp"
#include "words/linconstraint.hpp"

namespace Words {
  namespace Z3 {

	
	void setStr3 () {
	  Z3_global_param_set ("smt.string_solver","z3str3");
	}
	
	  
	class Z3Solver : public Words::SMT::Solver {
	public:
	  Z3Solver () {
		cfg = Z3_mk_config ();
		context = Z3_mk_context(cfg);
		solver = Z3_mk_solver(context);
		Z3_solver_inc_ref(context, solver);
		strsort = Z3_mk_string_sort (context);
		intsort = Z3_mk_int_sort (context);
	  }
	  
	  virtual ~Z3Solver () {
		Z3_solver_dec_ref(context, solver);
		Z3_del_config(cfg);
		Z3_del_context(context);
	  }
		
	  
	  virtual Words::SMT::SolverResult solve () {
	    if (timeout) {
		  Z3_params solverParams = Z3_mk_params(context);
		  Z3_params_inc_ref(context, solverParams);
		  Z3_symbol timeoutParamStrSymbol = Z3_mk_string_symbol(context, "timeout");
		  // "timeout (unsigned int) timeout (in milliseconds) (0 means no timeout) (default: 0)"
		  Z3_params_set_uint(context,
							 solverParams,
							 timeoutParamStrSymbol,
							 timeout); 
		  Z3_solver_set_params(context, solver, solverParams);
		  Z3_params_dec_ref(context, solverParams);
		}
		switch (Z3_solver_check (context,solver)) {
		case Z3_L_TRUE:
		  model = Z3_solver_get_model (context,solver);
		  return Words::SMT::SolverResult::Satis;
		  break;
		case Z3_L_FALSE:
		  return Words::SMT::SolverResult::NSatis;
		  break;
		case Z3_L_UNDEF:
		default:
		  return Words::SMT::SolverResult::Unknown;
		}
	  }
	  
	  virtual void addVariable (Words::Variable* v){
		std::stringstream str;
		v->output (str);
		auto symb = Z3_mk_string_symbol (context,str.str().c_str());
		auto ast = Z3_mk_const (context,symb,strsort);
		asts.insert (std::make_pair (v,ast));
	  }
	  
	  virtual void addTerminal (Words::Terminal* t) {
		terminals.insert(t->getRepr ());
	  }

	  virtual void addConstraint (const Constraints::Constraint& l) {
		if (l.isLinear ()) {
		  std::vector<Z3_ast> ast;
		  auto lin = l.getLinconstraint ();
		  for (auto& mult : *lin) {
			Z3_ast muls[2];
			muls[0] = Z3_mk_seq_length (context,asts.at(mult.entry));
			muls[1] = Z3_mk_int (context,mult.number,intsort);
			ast.push_back (Z3_mk_mul (context,2,muls));
		  }
		  auto added = Z3_mk_add (context,ast.size(),ast.data ());
		  auto rhs = Z3_mk_int (context,lin->getRHS (),intsort);
		  auto comp = Z3_mk_le (context,added,rhs);
		  Z3_solver_assert (context,solver,comp);
		}
		else if (!l.isUnrestricted ())
		  throw Words::ConstraintUnsupported ();
	  }

	  virtual void evaluate (Words::Variable* v, Words::WordBuilder& wb) {
		Z3_ast ast;
		Z3_model_eval (context,model,asts.at(v),true,&ast);
		auto str = Z3_get_string (context,ast);
		PassthroughStream<Words::WordBuilder> stream (wb,terminals);
		while (*str != '\0') {
		  stream << *str;
		  str++;
		}
		
	  }
	  
	  
	  virtual void addEquation (const Words::Equation& eq) {



          if(eq.lhs.characters() == 0){
			std::cout << " xxx : " << eq.lhs << std::endl;
			
			std::cout << eq << std::endl;
			
          }

          if(eq.rhs.characters() == 0){
              std::cout << " yyy : "  << std::endl;
          }

		auto left = buildWordAst (eq.lhs);
		auto right = buildWordAst (eq.rhs);
		auto eqe = Z3_mk_eq (context,left,right);
		Z3_solver_assert (context,solver,eqe);
	  }

	  Z3_ast buildWordAst (const Words::Word& w) {
		std::vector<Z3_ast> asts;
		
		std::stringstream str;
		if (w.characters () == 0) {
		  return Z3_mk_seq_empty (context,strsort);
		}
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
		  return Z3_mk_seq_concat (context,asts.size(),asts.data());
		else
		  return asts[0];
		  
		
	  }

	  virtual std::string getVersionString () const {
		std::stringstream str;
		str << "Z3 " << Z3_get_full_version (); 
		return str.str();
	  }

	  void setTimeout (size_t t ) {timeout = t;}
	  
	private:
	  Z3_context context;
	  Z3_config cfg;
	  std::unordered_map<Words::IEntry*,Z3_ast> asts;
	  Z3_sort strsort;
	  Z3_sort intsort;
	  Z3_solver solver;
	  Z3_model model;
	  std::set<char> terminals;
	  size_t timeout;
	  };

	class Z3IntegerSolver : public Words::SMT::IntegerSolver {
	public:
	  Z3IntegerSolver () {	
	  	cfg = Z3_mk_config ();
		context = Z3_mk_context(cfg);
		solver = Z3_mk_solver(context);
		Z3_solver_inc_ref(context, solver);		
		intsort = Z3_mk_int_sort (context);
	  }
	  
	  virtual ~Z3IntegerSolver () {
		Z3_solver_dec_ref(context, solver);
		Z3_del_config(cfg);
		Z3_del_context(context);
	  }
	  
	  virtual void addVariable (const Words::Variable* v) {
		
		std::stringstream str;
		v->output (str);
		auto symb = Z3_mk_string_symbol (context,str.str().c_str());
		auto ast = Z3_mk_const (context,symb,intsort);
		auto zero = Z3_mk_int (context,0,intsort);
		auto comp = Z3_mk_ge (context,ast,zero);
		Z3_solver_assert (context,solver,comp);
		asts.insert (std::make_pair (v,ast));
	  }
	  
	  virtual void addConstraint (const Constraints::Constraint& l) {
	    if (l.isLinear ()) {
	      std::vector<Z3_ast> ast;
	      auto lin = l.getLinconstraint ();
	      for (auto& mult : *lin) {
		Z3_ast muls[2];
		muls[0] = asts.at(mult.entry);
		muls[1] = Z3_mk_int (context,mult.number,intsort);
		ast.push_back (Z3_mk_mul (context,2,muls));
	      }
	      auto added = Z3_mk_add (context,ast.size(),ast.data ());
	      auto rhs = Z3_mk_int (context,lin->getRHS (),intsort);
	      auto comp = Z3_mk_le (context,added,rhs);
	      
	      
	      Z3_solver_assert (context,solver,comp);
	      
	    }
	  }
	  
	  virtual Words::SMT::SolverResult solve () {
	    if (timeout) {
	      Z3_params solverParams = Z3_mk_params(context);
	      Z3_symbol timeoutParamStrSymbol = Z3_mk_string_symbol(context, "timeout");
	      Z3_params_inc_ref(context, solverParams);
	      Z3_params_set_uint(context,
				 solverParams,
				 timeoutParamStrSymbol,
				 timeout); 
	      Z3_solver_set_params(context, solver, solverParams);
	      Z3_params_dec_ref(context, solverParams);
	    }
	    
	    switch (Z3_solver_check (context,solver)) {
	    case Z3_L_TRUE:
	      model = Z3_solver_get_model (context,solver);
	      return Words::SMT::SolverResult::Satis;
	      break;
	    case Z3_L_FALSE:	
	      return Words::SMT::SolverResult::NSatis;
	      break;
	    case Z3_L_UNDEF:
	    default:		    
	      return Words::SMT::SolverResult::Unknown;
	    }
	  }
	  
	  virtual size_t evaluate (Words::Variable* v)  {
		Z3_ast ast;
		Z3_model_eval (context,model,asts.at(v),true,&ast);
		unsigned res = 0;
		Z3_get_numeral_uint (context,ast,&res);
		return res;
	  }

	  void setTimeout (size_t t ) {timeout = t;}
	  
	private:
	  Z3_context context;
	  Z3_config cfg;
	  std::unordered_map<const Words::IEntry*,Z3_ast> asts;
	  Z3_sort intsort;
	  Z3_solver solver;
	  Z3_model model;
	  std::set<char> terminals;
	  size_t timeout = 0; 
	};
	
	Words::SMT::Solver_ptr makeZ3Solver () {
	  return std::make_unique<Z3Solver> ();
	}
	Words::SMT::IntSolver_ptr makeZ3IntSolver () {
	  return std::make_unique<Z3IntegerSolver> ();
	}
  }
}
  
