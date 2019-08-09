#include <sstream>
#include <unordered_map>
#include "parser/parsing.hpp"
#include "smtparser/parser.hpp"
#include "smtparser/ast.hpp"
#include "words/linconstraint.hpp"
#include "words/exceptions.hpp"

#include "core/Solver.h"

#include "util.hpp"

namespace Words {

  class JGenerator : public Words::JobGenerator  {
  public:
	JGenerator ()  {

	}
	std::unique_ptr<Words::Job> newJob ()  {
	  if (solver.solve ()) {
		auto  job  = std::make_unique<Job> ();
		job->options.context= context;
		Glucose::vec<Glucose::Lit> clause;
		for (auto it : eqs) {
		  
		  auto val = solver.modelValue(it.first);
		  
		  if (val == l_True) {
			clause.push(~Glucose::mkLit (it.first));
		  }
		  else if (val == l_False) {
			clause.push(Glucose::mkLit (it.first));
		  }
		  if (val == l_True) {
			job->options.equations.push_back (it.second);
		  }
		  
		}
		solver.addClause (clause);
		
		return std::move(job);
	  }
	  else
		return nullptr;
	}
	std::shared_ptr<Words::Context> context = nullptr;
	Glucose::Solver solver;
	std::unordered_map<Glucose::Var,Words::Equation> eqs;
	std::unordered_map<Glucose::Var,Words::Constraints::Constraint_ptr> constraints;	
  };
  
  
  template<typename T>
  class AutoNull {
  public:
	AutoNull (std::unique_ptr<T>& ptr) : ptr(ptr) {}
	~AutoNull () {ptr = nullptr;}
  private:
	std::unique_ptr<T>& ptr;
  };
  
  class TerminalAdder : public BaseVisitor {
  public:
	TerminalAdder (Words::Context& opt)  : ctxt(opt) {}
	virtual void caseStringLiteral (StringLiteral& s) {
	  for (auto c : s.getVal ()) {
		ctxt.addTerminal (c);
	}
	}
	virtual void caseAssert (Assert& c)
	{
	  c.getExpr()->accept(*this);
						  
	}

  private:
	Words::Context& ctxt;
  };

  template<class T>
  class Adder {
  public:
	Adder (std::unique_ptr<T>& t)  : inner(t),left(true) {}
	void add (int64_t val) {
	  if (left)
		inner->addLHS (val);
	  else
		inner->addRHS (val);
	}

	void add (Words::Constraints::VarMultiplicity& v) {
	  if (left)
		inner->addLHS (v.entry,v.number);
	  else
		inner->addRHS (v.entry,v.number);
	}

	void switchSide () {left = !left;}
	
  private:
	std::unique_ptr<T>& inner;
	bool left;
  };
  
  class LengthConstraintBuilder : public BaseVisitor {
  public:
	LengthConstraintBuilder (
							 Words::Context&ctxt,
							 Glucose::Solver& solver,
							 std::unordered_map<Glucose::Var,Words::Constraints::Constraint_ptr>& c
							 )  : ctxt(ctxt),
								  solver(solver),
								  constraints(c)
	{}

	template<Words::Constraints::Cmp cmp>
	void visitRedirect (DummyApplication& c) {
	  var = var_Undef;
	  if (c.getExpr (0)->getSort () == Sort::Integer) {
		builder = Words::Constraints::makeLinConstraintBuilder (cmp);
		adder = std::make_unique< Adder <Words::Constraints::LinearConstraintBuilder> > (builder);
		c.getExpr(0)->accept(*this);
		adder->switchSide ();
		c.getExpr(1)->accept(*this);
		//opt.constraints.push_back(builder->makeConstraint ());
		builder = nullptr;
		var = solver.newVar ();
	  }
	}
	
	virtual void caseLEQ (LEQ& c)
	{
	  visitRedirect<Words::Constraints::Cmp::LEq> (c);
	}
	
	
	virtual void caseLT (LT& c)
	{
	  visitRedirect<Words::Constraints::Cmp::Lt> (c);
	}
	
	virtual void caseGEQ (GEQ& c)
	{
	  visitRedirect<Words::Constraints::Cmp::GEq> (c);
	}
	
	virtual void caseGT (GT& c)
	{
	  visitRedirect<Words::Constraints::Cmp::Gt> (c);
	}
	
	virtual void caseEQ (EQ& c)
	{
	  visitRedirect<Words::Constraints::Cmp::LEq> (c);
	  visitRedirect<Words::Constraints::Cmp::GEq> (c);
	  
	}

	virtual void caseNumericLiteral (NumericLiteral& c) {
	
	  if (!vm) {
		adder->add (c.getVal ());
	  }
	  else {
		vm->number = c.getVal ();
	  }
	}

	virtual void caseMultiplication (Multiplication& c)
	{
	  Words::Constraints::VarMultiplicity kk (nullptr,1);
	  vm = &kk;
	  for (auto& cc : c)
		cc->accept (*this);
	  assert(kk.entry);
	  adder->add (kk);
	  vm = nullptr;
	}
	
	virtual void caseStringLiteral (StringLiteral& ) {}
	virtual void caseIdentifier (Identifier& c) {
	  entry = ctxt.findSymbol (c.getSymbol()->getVal());
  }

	virtual void caseAssert (Assert& c) {
	  c.getExpr()->accept(*this);
	  if (var != var_Undef) {
		solver.addClause (Glucose::mkLit (var));
	  }
	}

	virtual void caseStrLen (StrLen& c) {
	  c.getExpr (0)->accept(*this);
	  if (vm) {
		vm->entry = entry;
		entry= nullptr;
	  }
	  else {
		Words::Constraints::VarMultiplicity kk (entry,1);
		adder->add (kk);
		entry = nullptr;
	  }
	}

	virtual void caseDisjunction (Disjunction& c)
	{
	  Glucose::vec<Glucose::Lit> vec;
	  for (auto cc : c) {
		cc->accept (*this);
		if (var != var_Undef)
		  vec.push(Glucose::mkLit (var));
	  }
	  if (vec.size()) {
		var = solver.newVar ();
		reify_or (solver,Glucose::mkLit (var),vec);
		
	  }
	  else
		var = var_Undef;
	}
	
	virtual void caseConjunction (Conjunction& c)
	{
	  Glucose::vec<Glucose::Lit> vec;
	  for (auto cc : c) {
		cc->accept (*this);
		if (var != var_Undef)
		  vec.push(Glucose::mkLit (var));
	  }
	  if (vec.size()) {
		var = solver.newVar ();
		reify_and (solver,Glucose::mkLit (var),vec);
		
	  }
	  else
		var = var_Undef;
	}

   
  private:
	Words::Context& ctxt;
	std::unique_ptr<Words::Constraints::LinearConstraintBuilder> builder;
	Words::Constraints::VarMultiplicity* vm = nullptr;
	std::unique_ptr<Adder<Words::Constraints::LinearConstraintBuilder>> adder;
	IEntry* entry = nullptr;
	Glucose::Solver& solver;
	std::unordered_map<Glucose::Var,Words::Constraints::Constraint_ptr>& constraints;
	Glucose::Var var = var_Undef;
  };
  
  class StrConstraintBuilder : public BaseVisitor {
  public:
	StrConstraintBuilder (
						  Words::Context& ctxt,
						  Glucose::Solver& s,std::unordered_map<Glucose::Var,Words::Equation>& eq)  : 
																									  ctxt(ctxt),
																									  
																									  solver(s),
																									  eqs(eq)
	{}
	virtual void caseStringLiteral (StringLiteral& s) {
	  
	  for (auto c : s.getVal ()) {
		*wb << c;
	  }
	}

	virtual void caseIdentifier (Identifier& i) {
	  if (i.getSort () == Sort::String) {
		auto symb = i.getSymbol ();
		*wb << symb->getVal ();
	  }
	}
	
	virtual void caseAssert (Assert& c) {

	  c.getExpr()->accept(*this);
	  assert(var != var_Undef);
	  if (var != var_Undef)  {
		std::cerr << "ADD Clause" << std::endl;
		solver.addClause (Glucose::mkLit (var));
	  }
	}

	virtual void caseLEQ (LEQ& c)
	{
	  
	}
	
	virtual void caseLT (LT& c)
	{
	  
	}
	
	virtual void caseGEQ (GEQ& c)
	{
	  
	}
	
	virtual void caseGT (GT& c)
	{
	  
	}
	
	virtual void caseEQ (EQ& c)
	{
	  var = var_Undef;
	  auto lexpr = c.getExpr (0);
	  auto rexpr = c.getExpr (1);
	  if (lexpr->getSort () == Sort::String) {
		Words::Word left;
		Words::Word right;
		AutoNull<Words::WordBuilder> nuller (wb);
		wb = ctxt.makeWordBuilder (left);
		lexpr->accept(*this);
		wb->flush();
		wb = ctxt.makeWordBuilder (right);
		rexpr->accept(*this);
		wb->flush();
		//opt.equations.emplace_back(left,right);
		//opt.equations.back().ctxt = opt.context.get();

		Words::Equation eq (left,right);
		eq.ctxt = &ctxt;
		
		var = solver.newVar();
		eqs.insert(std::make_pair (var,eq));
	  }
	}

	virtual void caseNEQ (NEQ& c)
	{
	  var = var_Undef;
	  auto lexpr = c.getExpr (0);
	  auto rexpr = c.getExpr (1);
	  if (lexpr->getSort () == Sort::String) {
		Words::Word left;
		Words::Word right;
		AutoNull<Words::WordBuilder> nuller (wb);
		wb = ctxt.makeWordBuilder (left);
		lexpr->accept(*this);
		wb->flush();
		wb = ctxt.makeWordBuilder (right);
		rexpr->accept(*this);
		wb->flush();
		//opt.equations.emplace_back(left,right);
		//opt.equations.back().ctxt = opt.context.get();

		Words::Equation eq (left,right,Words::Equation::EqType::NEq);
		eq.ctxt = &ctxt;
		
		var = solver.newVar();
		eqs.insert(std::make_pair (var,eq));
	  }
	}
	
	
	virtual void casePlus (Plus& c)
	{
	  
	}
	
	virtual void caseMultiplication (Multiplication& c)
	{
	  
	}

	virtual void caseDisjunction (Disjunction& c)
	{
	  Glucose::vec<Glucose::Lit> vec;
	  for (auto cc : c) {
		cc->accept (*this);
		if (var != var_Undef)
		  vec.push(Glucose::mkLit (var));
	  }
	  if (vec.size()) {
		var = solver.newVar ();
		reify_or (solver,Glucose::mkLit (var),vec);
		
	  }
	  else
		var = var_Undef;
	}

	virtual void caseConjunction (Conjunction& c)
	{
	  Glucose::vec<Glucose::Lit> vec;
	  for (auto cc : c) {
		cc->accept (*this);
		if (var != var_Undef)
		  vec.push(Glucose::mkLit (var));
	  }
	  if (vec.size()) {
		var = solver.newVar ();
		reify_and (solver,Glucose::mkLit (var),vec);
		
	  }
	  else
		var = var_Undef;
	}
	
	
	virtual void caseStrLen (StrLen& c)
	{
	}
	
  private:
	Words::Context& ctxt;
	std::unique_ptr<WordBuilder> wb;
	Glucose::Solver& solver;
	std::unordered_map<Glucose::Var,Words::Equation>& eqs;
	Glucose::Var var = var_Undef;
	
  };
  
  class MyErrorHandler : public SMTParser::ErrorHandler {
  public:
	MyErrorHandler (std::ostream& err) : os(err){
	}
	virtual void error (const std::string& err) {
	  herror = true;
	  os  << err << std::endl;
	}

	virtual bool hasError () {return herror;}
  private:
	std::ostream& os;
	bool herror = false;
  };

  class MyParser : public ParserInterface {
  public:
	
	MyParser(std::istream& is) : str(is) {}
	virtual std::unique_ptr<Words::JobGenerator> Parse (std::ostream& err) {
	  auto jg = std::make_unique<JGenerator> ();
	  jg-> context = std::make_shared<Words::Context> ();
	  MyErrorHandler handler (err);
	  SMTParser::Parser parser;
	  parser.Parse (str,handler);
	  std::stringstream str;
	  for (auto& s : parser.getVars ()) {
		str << *s << std::endl;
		std::string ss = str.str();
		ss.pop_back();
		jg->context->addVariable (ss);
		str.str("");
	  }

	  TerminalAdder tadder(*jg->context);
	  StrConstraintBuilder sbuilder (*jg->context,jg->solver,jg->eqs);
	  LengthConstraintBuilder lbuilder(*jg->context,jg->solver,jg->constraints);
	  for (auto& t : parser.getAssert ()) {
		t->accept (tadder);
		t->accept (sbuilder);
		t->accept(lbuilder);
	  }
	  
	  return jg;
	}
	
  private:
	std::istream& str;
  };

  std::unique_ptr<ParserInterface> makeSMTParser (std::istream& is) {
	return std::make_unique<MyParser> (is);
  }
}
