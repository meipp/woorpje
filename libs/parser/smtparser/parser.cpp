#include <sstream>
#include "parser/parsing.hpp"
#include "smtparser/parser.hpp"
#include "smtparser/ast.hpp"
#include "words/linconstraint.hpp"



namespace Words {
  
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
	TerminalAdder (Words::Options& opt)  : opt(opt) {}
	virtual void caseStringLiteral (const StringLiteral& s) {
	  for (auto c : s.getVal ()) {
		opt.context->addTerminal (c);
	}
	}
	virtual void caseAssert (const Assert& c)
	{
	  c.getExpr()->accept(*this);
						  
	}

  private:
	Words::Options& opt;
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
	LengthConstraintBuilder (Words::Options& opt)  : opt(opt) {}

	template<Words::Constraints::Cmp cmp>
	void visitRedirect (const DummyApplication& c) {
	  if (c.getExpr (0)->getSort () == Sort::Integer) {
		builder = Words::Constraints::makeLinConstraintBuilder (cmp);
		adder = std::make_unique< Adder <Words::Constraints::LinearConstraintBuilder> > (builder);
		c.getExpr(0)->accept(*this);
		adder->switchSide ();
		c.getExpr(1)->accept(*this);
		opt.constraints.push_back(builder->makeConstraint ());
		builder = nullptr;
	  }
	}
	
	virtual void caseLEQ (const LEQ& c)
	{
	  visitRedirect<Words::Constraints::Cmp::LEq> (c);
	}
	
	
	virtual void caseLT (const LT& c)
	{
	  visitRedirect<Words::Constraints::Cmp::Lt> (c);
	}
	
	virtual void caseGEQ (const GEQ& c)
	{
	  visitRedirect<Words::Constraints::Cmp::GEq> (c);
	}
	
	virtual void caseGT (const GT& c)
	{
	  visitRedirect<Words::Constraints::Cmp::Gt> (c);
	}
	
	virtual void caseEQ (const EQ& c)
	{
	  visitRedirect<Words::Constraints::Cmp::LEq> (c);
	  visitRedirect<Words::Constraints::Cmp::GEq> (c);
	  
	}

	virtual void caseNumericLiteral (const NumericLiteral& c) {
	
	  if (!vm) {
		adder->add (c.getVal ());
	  }
	  else {
		vm->number = c.getVal ();
	  }
	}

	virtual void caseMultiplication (const Multiplication& c)
	{
	  Words::Constraints::VarMultiplicity kk (nullptr,1);
	  vm = &kk;
	  for (auto& cc : c)
		cc->accept (*this);
	  assert(kk.entry);
	  adder->add (kk);
	  vm = nullptr;
	}
	
	virtual void caseStringLiteral (const StringLiteral& ) {}
	virtual void caseIdentifier (const Identifier& c) {
	  entry = opt.context->findSymbol (c.getSymbol()->getVal());
	}

	virtual void caseAssert (const Assert& c) {
	  c.getExpr()->accept(*this);
	}

	virtual void caseStrLen (const StrLen& c) {
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

	
	virtual void caseDisjunction (const Disjunction& c)
	{
	  
	}
  private:
	Words::Options& opt;
	std::unique_ptr<Words::Constraints::LinearConstraintBuilder> builder;
	Words::Constraints::VarMultiplicity* vm = nullptr;
	std::unique_ptr<Adder<Words::Constraints::LinearConstraintBuilder>> adder;
	IEntry* entry = nullptr;
  };
  
  class StrConstraintBuilder : public BaseVisitor {
  public:
	StrConstraintBuilder (Words::Options& opt)  : opt(opt) {}
	virtual void caseStringLiteral (const StringLiteral& s) {
	  
	  for (auto c : s.getVal ()) {
		*wb << c;
	  }
	}

	virtual void caseIdentifier (const Identifier& i) {
	  if (i.getSort () == Sort::String) {
		auto symb = i.getSymbol ();
		*wb << symb->getVal ();
	  }
	}
	
	virtual void caseAssert (const Assert& c) {
	  c.getExpr()->accept(*this);
	}

	virtual void caseLEQ (const LEQ& c)
	{
	  
	}
	
	virtual void caseLT (const LT& c)
	{
	  
	}
	
	virtual void caseGEQ (const GEQ& c)
	{
	  
	}
	
	virtual void caseGT (const GT& c)
	{
	  
	}
	
	virtual void caseEQ (const EQ& c)
	{
	  auto lexpr = c.getExpr (0);
	  auto rexpr = c.getExpr (1);
	  if (lexpr->getSort () == Sort::String) {
		Words::Word left;
		Words::Word right;
		AutoNull<Words::WordBuilder> nuller (wb);
		wb = opt.context->makeWordBuilder (left);
		lexpr->accept(*this);
		wb->flush();
		wb = opt.context->makeWordBuilder (right);
		rexpr->accept(*this);
		wb->flush();
		opt.equations.emplace_back(left,right);
		opt.equations.back().ctxt = opt.context.get();
	  }
	}
	
	virtual void casePlus (const Plus& c)
	{
	  
	}
	
	virtual void caseMultiplication (const Multiplication& c)
	{
	  
	}

	virtual void caseDisjunction (const Disjunction& c)
	{
	  
	}
	
	virtual void caseStrLen (const StrLen& c)
	{
	}
	
  private:
	Words::Options& opt;
	std::unique_ptr<WordBuilder> wb;
  };
  
  class MyErrorHandler : public SMTParser::ErrorHandler {
  public:
	MyErrorHandler (std::ostream& err) : os(err){
	}
	virtual void error (const std::string& err) {
	  herror = true;
	  os  << err << std::endl;
	  throw std::runtime_error ("HH");
	}

	virtual bool hasError () {return herror;}
  private:
	std::ostream& os;
	bool herror = false;
  };

  class MyParser : public ParserInterface {
  public:
	
	MyParser(std::istream& is) : str(is) {}
	virtual Solvers::Solver_ptr Parse (Words::Options& opt,std::ostream& err) {
	  opt.context = std::make_shared<Words::Context> ();
	  MyErrorHandler handler (err);
	  SMTParser::Parser parser;
	  parser.Parse (str,handler);
	  std::stringstream str;
	  for (auto& s : parser.getVars ()) {
		str << *s << std::endl;
		std::string ss = str.str();
		ss.pop_back();
		opt.context->addVariable (ss);
		str.str("");
	  }

	  TerminalAdder tadder(opt);
	  StrConstraintBuilder sbuilder (opt);
	  LengthConstraintBuilder lbuilder(opt);
	  for (auto& t : parser.getAssert ()) {
		t->accept (tadder);
		t->accept (sbuilder);
		t->accept(lbuilder);
	  }
	  
	  return nullptr;
	}
	
  private:
	std::istream& str;
  };

  std::unique_ptr<ParserInterface> makeSMTParser (std::istream& is) {
	return std::make_unique<MyParser> (is);
  }
}
