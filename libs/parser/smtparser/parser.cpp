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
      std::stringstream str;
      static int i = 0;
	  
      if (solver.solve ()) {
		UpdateSolverBuilder builder (hashToLit,constraints,eqs,solver,neqmap);
		for (auto& t : parser.getAssert ()) {
		  builder.Run(*t);
		}
		auto job =  builder.finalise ();
		job->options.context = context;;
		return job;
	  }
	  
      else
		return nullptr;
    }
    std::shared_ptr<Words::Context> context = nullptr;
    Glucose::Solver solver;
    Glucose::vec<Glucose::Lit> assumptions;
    std::unordered_map<Glucose::Var,Words::Equation> eqs;
    std::unordered_map<Glucose::Var,Words::Constraints::Constraint_ptr> constraints;
	std::unordered_map<size_t, Glucose::Lit> hashToLit;
	SMTParser::Parser parser;
	std::unordered_map<NEQ*,ASTNode_ptr> neqmap;
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

    virtual void caseFunctionApplication ( FunctionApplication& c) {
      throw UnsupportedFeature();
    }
    
  private:
    Words::Context& ctxt;
  };

  
  class LengthConstraintBuilder : public BaseVisitor {
  public:
    LengthConstraintBuilder (
			     Words::Context&ctxt,
			     Glucose::Solver& solver,
			     std::unordered_map<Glucose::Var,Words::Constraints::Constraint_ptr>& c,
			     std::unordered_map<Glucose::Var,Words::Equation>&eqs,
				 std::unordered_map<size_t, Glucose::Lit>& hlit,
				 std::unordered_map<NEQ*,ASTNode_ptr>& neqmap
							 )  : ctxt(ctxt),
								  solver(solver),
								  constraints(c),eqs(eqs),
								  alreadyCreated (hlit),
								  neqmap(neqmap)
    {}

    template<Words::Constraints::Cmp cmp>
    void visitRedirect (DummyApplication& c) {
      if (!checkAlreadyIn (c,var)) {
	 var = Glucose::lit_Undef;
	 if (c.getExpr (0)->getSort () == Sort::Integer) {
	   builder = Words::Constraints::makeLinConstraintBuilder (cmp);
	   adder = std::make_unique< Adder <Words::Constraints::LinearConstraintBuilder> > (builder);
	   AutoNull < Adder <Words::Constraints::LinearConstraintBuilder> > nuller (adder);
	   c.getExpr(0)->accept(*this);
	   adder->switchSide ();
	   c.getExpr(1)->accept(*this);
	   
	   
	   auto v = solver.newVar (); 
	   var = Glucose::mkLit (v);
	   constraints.insert (std::make_pair(v,builder->makeConstraint ()));
	   builder = nullptr;
	   insert(c,var);
	 }
	 
       }
    }

    void Run (ASTNode& m) {
      m.accept (*this);
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

      if (!checkAlreadyIn (c,var)) {
	
	
	var = Glucose::lit_Undef;
	auto lexpr = c.getExpr (0);
	auto rexpr = c.getExpr (1);
	if (lexpr->getSort () == Sort::String) {
	  if (equalities.count (&c)) {
	    var = equalities.at (&c);
	  }
	  else {
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
	    var = makeEquLit (c);
	    eqs.insert(std::make_pair (Glucose::var(var),eq));
	    
	    
	  }
	  insert(c,var);
	}
	else if (lexpr->getSort () == Sort::Integer)  {
	  //Throw for now,  as the visitRedirect cannot handle LEQ and GEQ simultaneously
	  throw UnsupportedFeature ();
	  visitRedirect<Words::Constraints::Cmp::LEq> (c);
	  visitRedirect<Words::Constraints::Cmp::GEq> (c);
	}
	
	else if (lexpr->getSort () == Sort::Bool) {
	  throw Words::WordException ("SHouldn't get here");
	}
      }
    }

    Glucose::Lit makeEquLit (EQ& c) {
      if (equalities.count (&c)) {
	return equalities.at ( &c);
      }
      auto v = solver.newVar ();
      auto var = Glucose::mkLit (v);
      equalities.insert(std::make_pair(&c,var));
      return var;
    }
    
    virtual void caseNEQ (NEQ& c)
    {
      if (!checkAlreadyIn (c,var)) {

	
      
	var = Glucose::lit_Undef;
	auto lexpr = c.getExpr (0);
	auto rexpr = c.getExpr (1);
	if (lexpr->getSort () == Sort::String) {
	  auto eqLit = makeEquLit (*c.getInnerEQ ());
	  //WE need to grab the inner equality,
	  //negate the literal associated to it (pass that upwards)
	  //and add a biimplication to the disjunction created below.
	  
	  static size_t i = 0;
	
	  std::stringstream str;
	  str << "_woorpje_diseq_pref" << i;
	  auto symb_pref = std::make_shared<Symbol> (str.str());
	  ctxt.addVariable (str.str());
	  str.str("");
	  str << "_woorpje_diseq_suf_l" << i;
	  auto symb_sufl = std::make_shared<Symbol> (str.str());
	  ctxt.addVariable (str.str());
	  str.str("");
	  str << "_woorpje_diseq_suf_r" << i;
	  i++;
	  auto symb_sufr = std::make_shared<Symbol> (str.str());
	  ctxt.addVariable (str.str());
	

	
		
	  std::vector<ASTNode_ptr> disjuncts;
	  if (ctxt.getTerminalAlphabet ().size () > 2) {
	    //Greater than 2, because it only makes to distinguish the middle character,
	    //when we have more than an epsilon and one extra character in the terminals
	    symb_pref->setSort (Sort::String);
	    symb_sufl->setSort (Sort::String);
	    symb_sufr->setSort (Sort::String);
	  
	  
	    ASTNode_ptr Z = std::make_shared<Identifier> (*symb_pref);
	    ASTNode_ptr X = std::make_shared<Identifier> (*symb_sufl);
	    ASTNode_ptr Y = std::make_shared<Identifier> (*symb_sufr);
	  
	  
	    for (auto a : ctxt.getTerminalAlphabet ()) {
	      if (a->getRepr () == '_')
		continue;
	      std::vector<ASTNode_ptr> innerdisjuncts;
	      ASTNode_ptr strl = std::make_shared<StringLiteral> (std::string(1,a->getRepr ()));
	      auto concat = std::make_shared<StrConcat> (std::initializer_list<ASTNode_ptr> ({Z,strl,X}));
	      ASTNode_ptr outeq = std::make_shared<EQ> (std::initializer_list<ASTNode_ptr> ({lexpr,concat})); 
	      for (auto b : ctxt.getTerminalAlphabet ()) {
		if (b == a || b->getRepr () == '_')
		  continue;
		ASTNode_ptr strr = std::make_shared<StringLiteral> (std::string(1,b->getRepr ()));
		ASTNode_ptr concat_nnner = std::make_shared<StrConcat> (std::initializer_list<ASTNode_ptr> ({Z,strr,Y}));
		ASTNode_ptr in_eq = std::make_shared<EQ> ( std::initializer_list<ASTNode_ptr> ({rexpr,concat_nnner}));
		innerdisjuncts.push_back (in_eq);
	      }
	      ASTNode_ptr disj = std::make_shared<Disjunction> (std::move(innerdisjuncts));
	      disjuncts.push_back (std::make_shared<Conjunction> (std::initializer_list<ASTNode_ptr> ({outeq,disj})));
	    
	    }
	  }
		
	
	  ASTNode_ptr llength = std::make_shared<StrLen> (std::initializer_list<ASTNode_ptr> ({lexpr})); 
	  ASTNode_ptr rlength = std::make_shared<StrLen> (std::initializer_list<ASTNode_ptr> ({rexpr}));
	  ASTNode_ptr gt = std::make_shared<GT> (std::initializer_list<ASTNode_ptr> ({llength,rlength}));
	  ASTNode_ptr lt = std::make_shared<LT> (std::initializer_list<ASTNode_ptr> ({llength,rlength}));
	
	  disjuncts.push_back (gt);
	  disjuncts.push_back (lt);
	
	
	  ASTNode_ptr outdisj = std::make_shared<Disjunction> (std::move(disjuncts));
	  outdisj->accept(*this);

	  Glucose::vec<Glucose::Lit> clause;
	  clause.push (~eqLit);
	  reify_and_bi (solver,var,clause);
	  insert(c,var);
	  neqmap.insert(std::make_pair(&c,outdisj));
	}
	else {
	  throw Words::UnsupportedFeature ();
	}
      }
    }

    virtual void caseNumericLiteral (NumericLiteral& c) {
	
      if (!vm) {
	adder->add (c.getVal ());
      }
      else {
	vm->number = c.getVal ();
      }
    }

    virtual void caseFunctionApplication (FunctionApplication& c) {
      throw UnsupportedFeature ();
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
	
    virtual void caseStringLiteral (StringLiteral& s) {
      if (adder) {
	adder->add (s.getVal ().size());
      }
      else
	for (auto c : s.getVal ()) {
	  *wb << c;
	}
    }
    virtual void caseIdentifier (Identifier& c) {
      if (c.getSort () == Sort::String && instrlen  ) {
	if (vm)
	  vm->entry = ctxt.findSymbol (c.getSymbol()->getVal());
	else 
	  entry = ctxt.findSymbol (c.getSymbol()->getVal());
      }
      else if (c.getSort () == Sort::String && !instrlen) {
	auto symb = c.getSymbol ();
	*wb << symb->getVal ();
      }
      else if (c.getSort () == Sort::Integer) {
		UnsupportedFeature ();
	  }
      else if (c.getSort ()== Sort::Bool) {
	if (!checkAlreadyIn (c,var)) {
	  if (boolvar.count(&c)) {
	    var = boolvar.at(&c);
	  }
	  else {
	    auto v = solver.newVar ();
	    var = Glucose::mkLit (v);
	    boolvar.insert(std::make_pair (&c,var));
	  }
	  insert(c,var);
	}
	
      }
      else
	throw Words::WordException ("Error hh");
    }
	
    virtual void caseNegLiteral (NegLiteral& c) override {
      if (!checkAlreadyIn (c,var)) { 
	c.inner()->accept(*this);
	var = ~var;
	insert(c,var);
      }
    }
	
    virtual void caseAssert (Assert& c) {
      c.getExpr()->accept(*this);
      if (var != Glucose::lit_Undef) {
	solver.addClause (var);
      }
    }
	
    virtual void caseStrLen (StrLen& c) {
	instrlen = true;
	c.getExpr (0)->accept(*this);
	instrlen = false;
	if (entry && !vm) {	
	  Words::Constraints::VarMultiplicity kk (entry,1);
	  adder->add (kk);
	  entry = nullptr;
	}
    }

    virtual void caseStrConcat ( StrConcat& c)
    {
	for (auto& cc : c) {
	  cc->accept (*this);
	  if (instrlen) {
	    if (entry) {
	      Words::Constraints::VarMultiplicity kk (entry,1);
	      adder->add (kk);
	    entry = nullptr;
	    }
	  }
	}
    }
	
    virtual void caseDisjunction (Disjunction& c)
    {
      if (!checkAlreadyIn (c,var)) { 
	Glucose::vec<Glucose::Lit> vec;
	for (auto cc : c) {
	  cc->accept (*this);
	  if (var != Glucose::lit_Undef)
	    vec.push(var);
	}
	if (vec.size()) {
	  auto v = solver.newVar ();
	  var = Glucose::mkLit(v);
	  reify_or_bi (solver,var,vec);
	  
	  insert(c,var);
	}
	else
	  var = Glucose::lit_Undef;
      }
    }
	
    virtual void caseConjunction (Conjunction& c)
    {
      if (!checkAlreadyIn (c,var)) {
	Glucose::vec<Glucose::Lit> vec;
	for (auto cc : c) {
	  cc->accept (*this);
	  if (var != Glucose::lit_Undef)
	    vec.push(var);
	}
	if (vec.size()) {
	  auto v = solver.newVar();
	  var = Glucose::mkLit (v);
	  reify_and_bi (solver,var,vec);
	  insert(c,var);
	}
	else
	  var = Glucose::lit_Undef;
	
      }
    }

   
  private:

    bool checkAlreadyIn (ASTNode& n,Glucose::Lit& l) {
      l = Glucose::lit_Undef;
      auto h = n.hash ();
      if (alreadyCreated.count (h)){
	l = alreadyCreated.at (h);
	return true;
      }
      return false;
    }

    void insert (ASTNode& n, Glucose::Lit& l) {
      alreadyCreated.insert(std::make_pair (n.hash(),l));
    }
    
    Words::Context& ctxt;
    std::unique_ptr<Words::Constraints::LinearConstraintBuilder> builder;
    Words::Constraints::VarMultiplicity* vm = nullptr;
    std::unique_ptr<Adder<Words::Constraints::LinearConstraintBuilder>> adder;
    IEntry* entry = nullptr;
    Glucose::Solver& solver;
    std::unordered_map<Glucose::Var,Words::Constraints::Constraint_ptr>& constraints;
    Glucose::Lit var = Glucose::lit_Undef;

    std::unique_ptr<WordBuilder> wb;
    std::unordered_map<Glucose::Var,Words::Equation>& eqs;
    std::unordered_map<void*,Glucose::Lit> boolvar;

    std::unordered_map<EQ*,Glucose::Lit> equalities;
    
    bool instrlen = false;
    std::unordered_map<size_t, Glucose::Lit>& alreadyCreated;
	std::unordered_map<NEQ*,ASTNode_ptr>& neqmap;
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
      jg->parser.Parse (str,handler);
      std::stringstream str;
      for (auto& s : jg->parser.getVars ()) {
		if (s->getSort () == Sort::String) {
		  str << *s << std::endl;
		  std::string ss = str.str();
		  ss.pop_back();
		  jg->context->addVariable (ss);
		  str.str("");
		}
		
      }
	  
      TerminalAdder tadder(*jg->context);
	  
      LengthConstraintBuilder lbuilder(*jg->context,jg->solver,jg->constraints,jg->eqs,jg->hashToLit,jg->neqmap);
      for (auto& t : jg->parser.getAssert ()) {
		t->accept (tadder);
		lbuilder.Run (*t);	
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
