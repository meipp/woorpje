#include <cvc4/cvc4.h>
#include <memory>
#include <sstream>
#include <unordered_map>
#include "passthrough.hpp"
#include "words/exceptions.hpp"
#include "smt/smtsolvers.hpp"
#include "words/linconstraint.hpp"


namespace Words {
  namespace CVC4 {
	class CVC4Solver : public Words::SMT::Solver {
	public:
	  CVC4Solver () : engine(&em) {
		engine.setOption("produce-models", "true");
		engine.setOption("strings-exp", "true");
		// Set output language to SMTLIB2
		engine.setOption("output-language", "smt2");
	  }
	  
	  virtual ~CVC4Solver () {}

	  virtual Words::SMT::SolverResult solve () {
		if (asserts.size () > 0) {
		  auto form = asserts[0];
		  for (size_t i = 0; i < asserts.size(); i++) {
			form = em.mkExpr (::CVC4::kind::AND,form,asserts[i]);
		  }
		  
		  auto res = engine.checkSat (form);
		  switch (res.isSat ()) {
		  case ::CVC4::Result::Sat::UNSAT:
			return Words::SMT::SolverResult::NSatis;
			break;
		  case ::CVC4::Result::Sat::SAT:
			return Words::SMT::SolverResult::Satis;
			break;
		  case ::CVC4::Result::Sat::SAT_UNKNOWN:
		  default:
			return Words::SMT::SolverResult::Unknown;
			break;
			}
		}
		else {
		  return Words::SMT::SolverResult::Unknown;
		}
	  }
	  
	  virtual void addVariable (Words::Variable* v){
		std::stringstream str;
		str << v->getRepr ();
		exprs.insert (std::make_pair (v,em.mkVar (str.str(),em.stringType())));
	  }
	  
	  virtual void addTerminal (Words::Terminal*t ) {
		terminals.insert (t->getRepr ());
	  }

	  virtual void addConstraint (const Constraints::Constraint& l) {
		if (l.isLinear ()) {
		  std::vector<::CVC4::Expr> ast;
		  auto lin = l.getLinconstraint ();
		  ::CVC4::Expr added;
		  bool first = true;
		  for (auto& mult : *lin) {
			
			//Z3_ast muls[2];
			auto  mul1 = em.mkExpr (::CVC4::kind::STRING_LENGTH,exprs.at (mult.entry));
			auto mult2 = em.mkConst (::CVC4::Rational (mult.number));
			auto l = em.mkExpr (::CVC4::kind::MULT,mul1,mult2);
			if (first) {
			  added = l;
			  first = false; 
			}
			else {
			  added = em.mkExpr (::CVC4::kind::PLUS,added,l);
			}
			//muls[0] = Z3_mk_seq_length (context,asts.at(mult.entry));
			//muls[1] = Z3_mk_int (context,mult.number,intsort);
			//ast.push_back (Z3_mk_mul (context,2,muls));
		  }
		  auto rhs = em.mkConst (::CVC4::Rational (lin->getRHS ()));
		  asserts.push_back (em.mkExpr ( ::CVC4::kind::LEQ,added,rhs));
		  /*auto added = Z3_mk_add (context,ast.size(),ast.data ());
		  auto rhs = Z3_mk_int (context,lin->getRHS (),intsort);
		  auto comp = Z3_mk_le (context,added,rhs);
		  Z3_solver_assert (context,solver,comp);*/
		}
		else 
		  throw Words::ConstraintUnsupported ();
	  }

	  virtual void evaluate (Words::Variable* v, Words::WordBuilder& wb) {
		auto& e = exprs.at (v);
		std::stringstream str;
		str << engine.getValue (e);
		auto ss = str.str();
		auto it = ss.begin()+1; //Needed to filter out the 
		auto end = ss.end()-1; //leading " and ending "
		PassthroughStream<Words::WordBuilder> stream (wb,terminals);
		for (; it!=end; ++it ) {
		  stream << *it;
		}
	  }
	  
	  
	  virtual void addEquation (const Words::Equation& eq) {
		auto left = buildWordAst (eq.lhs);
		auto right = buildWordAst (eq.rhs);
		auto eqe = em.mkExpr (::CVC4::kind::EQUAL, left,right);
		asserts.push_back (eqe);
	  }

	  virtual std::string getVersionString () const {
		return "CVC4";
	  }

	  ::CVC4::Expr  buildWordAst (const Words::Word& w) {
		std::vector<::CVC4::Expr> asts;
		
		std::stringstream str;
		for (auto i : w) {
		  if (i->isVariable ()) {
			if (str.str().size ()) {
			  auto seq = em.mkConst (::CVC4::String (str.str().c_str()));
			  asts.push_back (seq);
			  str.str("");
			}
			asts.push_back ( this->exprs.at(i));
		  }
		  else {
			str << i->getRepr ();
		  }
		}
		
		if (str.str ().size()) {
		  auto seq = em.mkConst (::CVC4::String(str.str().c_str()));//Z3_mk_string (context,str.str().c_str());
		  asts.push_back (seq);
		}

		if (asts.size() > 1) 
		  return em.mkExpr (::CVC4::kind::STRING_CONCAT,asts); 
		else
		  return asts[0];
		  
		
	  }
	  
	private:
	  ::CVC4::ExprManager em;
	  ::CVC4::SmtEngine engine;
	  std::unordered_map<Words::IEntry*,::CVC4::Expr> exprs;
	  std::vector<::CVC4::Expr> asserts;
	  std::set<char> terminals;
	};
  
	Words::SMT::Solver_ptr makeCVC4Solver () {
	  return std::make_unique<CVC4Solver> ();
	}
  }
}
