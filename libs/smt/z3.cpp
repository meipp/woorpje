#include <z3.h>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <set>
#include "words/exceptions.hpp"
#include "smt/smtsolvers.hpp"
#include "words/linconstraint.hpp"

namespace Words {
  namespace Z3 {

	
	
	template<class Stream>
	struct PassthroughStream {
	  PassthroughStream (Stream& s, std::set<char>& terminals, char c = '_') : stream(s),
												   terminals(terminals),
												   dummy(c) {}
	  void operator<< (char c) {
		state (c,*this);
	  }

	private:
	  
	  static void standard (char c,PassthroughStream<Stream>& s) {

		switch (c) {
		case '\\':
		  s.state = first;
		  break;
		default:
		  s.push(c);
		}
	  }
	  
	  static void first (char c,PassthroughStream<Stream>& s) {
	
		switch (c) {
		case 'a':
		case 'b':
		case 'f':
		case 'n':
		case 'r':
		case 't':
		case 'v':
		  s.push (s.dummy);
		  s.state = standard;
		  break;
		case 'x':
		  s.state = second;
		  break;
		default:
		  s.push ('\\');
		  s.push (c);
		}
		
		
	  }
	  
	  static void second (char c,PassthroughStream<Stream>& s) {

		s.state = third;
	  }
	  
	  static void third (char c,PassthroughStream<Stream>& s) {

		s.stream << s.dummy;
		s.state = standard;
	  }

	  void push (char c) {
		if (terminals.count(c)) {
		  stream << c;
		}

		else {
		  stream << dummy;
		}
								  
	  }
	  
	  std::function<void(char,PassthroughStream<Stream>&)> state = standard;
	  Stream& stream;
	  std::set<char>& terminals;
	  char dummy;
	};
	  
	class Z3Solver : public Words::SMT::Solver {
	public:
	  Z3Solver () {
		cfg = Z3_mk_config ();
		context = Z3_mk_context(cfg);
		solver = Z3_mk_solver(context);
		strsort = Z3_mk_string_sort (context);
		intsort = Z3_mk_int_sort (context);
	  }
	  
	  virtual ~Z3Solver () {}
		
	  
	  virtual Words::SMT::SolverResult solve () {
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
		str << v->getRepr ();
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
		else 
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
		  return Z3_mk_seq_concat (context,asts.size(),asts.data());
		else
		  return asts[0];
		  
		
	  }

	private:
	  Z3_context context;
	  Z3_config cfg;
	  std::unordered_map<Words::IEntry*,Z3_ast> asts;
	  Z3_sort strsort;
	  Z3_sort intsort;
	  Z3_solver solver;
	  Z3_model model;
	  std::set<char> terminals;
	  };
  
	Words::SMT::Solver_ptr makeZ3Solver () {
	  return std::make_unique<Z3Solver> ();
	}
  }
}
  
