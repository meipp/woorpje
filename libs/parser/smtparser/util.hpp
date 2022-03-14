// lhs -> /\ rhs

namespace Words {

  void reify_and(Glucose::Solver & s, Glucose::Lit lhs, Glucose::vec<Glucose::Lit> & rhs){
    assert(rhs.size() > 0 && "reifying empty list? ");
    // lhs -> rhs[i]
    for(int i = 0 ; i < rhs.size();i++){
      Glucose::vec<Glucose::Lit> ps;
      ps.push(rhs[i]);
      ps.push(~lhs);
      s.addClause(ps);
    }
	
  }

  // lhs -> \/ rhs
  void reify_or(Glucose::Solver & s, Glucose::Lit lhs, Glucose::vec<Glucose::Lit> & rhs){
    // lhs -> \/ rhs
    Glucose::vec<Glucose::Lit> ps;
    for(int i = 0 ; i < rhs.size();i++)
      ps.push(rhs[i]);
    ps.push(~lhs);
    s.addClause(ps);
  }

  // lhs <-> /\ rhs
  void reify_and_bi(Glucose::Solver & s, Glucose::Lit lhs, Glucose::vec<Glucose::Lit> & rhs){
    assert(rhs.size() > 0 && "reifying empty list? ");
    // lhs -> rhs[i]
    for(int i = 0 ; i < rhs.size();i++){
      Glucose::vec<Glucose::Lit> ps;
      ps.push(rhs[i]);
      ps.push(~lhs);
      s.addClause(ps);
    }
    // /\rhs -> lhs
    Glucose::vec<Glucose::Lit> ps;
    for(int i = 0 ; i < rhs.size();i++)
      ps.push(~rhs[i]);
    ps.push(lhs);
    s.addClause(ps);
  }

  // lhs <-> \/ rhs
  void reify_or_bi(Glucose::Solver & s, Glucose::Lit lhs, Glucose::vec<Glucose::Lit> & rhs){
    assert(rhs.size() > 0 && "reifying empty list? ");
    // rhs[i] -> lhs
    for(int i = 0 ; i < rhs.size();i++){
      Glucose::vec<Glucose::Lit> ps;
      ps.push(~rhs[i]);
      ps.push(lhs);
      s.addClause(ps);
    }
    // lhs -> \/ rhs
    Glucose::vec<Glucose::Lit> ps;
    for(int i = 0 ; i < rhs.size();i++)
      ps.push(rhs[i]);
    ps.push(~lhs);
    s.addClause(ps);
  }

  
  void at_most_one (Glucose::Solver & s, Glucose::vec<Glucose::Lit> & rhs){
    for(int i = 0 ; i < rhs.size();i++){
      for (int j = i+1; j < rhs.size(); j++) {
        Glucose::vec<Glucose::Lit> ps;
        ps.push(~rhs[i]);
        ps.push(~rhs[j]);
        s.addClause(ps);
      }	  
    }
  }

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


  class UpdateSolverBuilder : public BaseVisitor {
  public:
    UpdateSolverBuilder(
			std::unordered_map<size_t, Glucose::Lit>& hlit,
			std::unordered_map<Glucose::Var,Words::Constraints::Constraint_ptr>& c,
			std::unordered_map<Glucose::Var,Words::Equation>& e,
			Glucose::Solver& s,
			std::unordered_map<size_t,ASTNode_ptr>& neqmap,
			Glucose::vec<Glucose::Lit>& assumptions
						)  : alreadyCreated (hlit),
							 constraints(c),
							 eqs(e),
							 solver(s),
							 neqmap(neqmap),
							 assumptions(assumptions)					  
    {}
	
    template<class T>

    void visitRedirect (T& c) {
      assert(alreadyCreated.count(c.hash()));
      auto l = Glucose::var(alreadyCreated.at(c.hash()));
      auto res = solver.modelValue(l);
      if (res == l_True) {
        if (constraints.count(l)) {
          job->options.constraints.push_back(constraints.at(l));
        }
        
        if (eqs.count(l)) {
          job->options.equations.push_back(eqs.at(l));
        }

        clause.push(~Glucose::mkLit(l));	  
      }
      else if (res == l_False) {
	clause.push(Glucose::mkLit(l));
      }
    }
	
    void Run (ASTNode& m) {
      m.accept (*this);
    }

    auto finalise () {
      auto var = solver.newVar ();
      auto lit = Glucose::mkLit (var);
      assumptions.push(lit);
      solver.addClause (clause);

      if (clause.size()) {
          reify_or_bi(solver, lit, clause);
      }
	  
	  
      return std::move(job);
    }
    
    virtual void caseLEQ (LEQ& c) override {
      visitRedirect (c);
    }
	
    virtual void caseLT (LT& c) override {
      visitRedirect (c);
    }
	
    virtual void caseGEQ (GEQ& c) override {
      visitRedirect (c);
    }
	
    virtual void caseGT (GT& c) override {
      visitRedirect (c);
    }
	
    virtual void caseEQ (EQ& c) override {
      visitRedirect(c);
    } 


    virtual void caseStrPrefixOf (StrPrefixOf& c) override {
      visitRedirect(c);
    }

    virtual void caseStrSuffixOf (StrSuffixOf& c) override {
      visitRedirect(c);
    }

    virtual void caseStrContains (StrContains& c) override {
      visitRedirect(c);
    }
    
    
    virtual void caseNEQ (NEQ& c) override {
      assert(neqmap.count(c.hash()));
      neqmap.at(c.hash())->accept (*this);
    } 
		
    virtual void caseFunctionApplication (FunctionApplication&) override {
      throw UnsupportedFeature ();
    }
    
    virtual void caseNegLiteral (NegLiteral& c) override {
      c.inner()->accept(*this);
    }

    virtual void caseAssert (Assert& c) override {
      //auto l = Glucose::var(alreadyCreated.at(c.getExpr()->hash()));
      c.getExpr()->accept(*this);
    }

    virtual void caseDisjunction (Disjunction& c) override {
      assert(alreadyCreated.count(c.hash()));
      auto l = Glucose::var(alreadyCreated.at(c.hash()));
      if (solver.modelValue (l) == l_True) {
        for (auto cc : c)  {
          auto ll = Glucose::var(alreadyCreated.at(cc->hash()));
          auto res = solver.modelValue (ll);
          if (res!=l_False) {
            cc->accept (*this);
          }
          if ( res== l_True)
            break;
        }
	      clause.push(~Glucose::mkLit(l));
      } else if (solver.modelValue (l) == l_False){
	      clause.push(Glucose::mkLit(l));
      }      
    }
    
    virtual void caseConjunction (Conjunction& c) override {
      assert(alreadyCreated.count(c.hash()));
      auto l = Glucose::var(alreadyCreated.at(c.hash()));
      if (solver.modelValue (l) == l_True) {
        for (auto cc : c)  {
          cc->accept (*this);
        }
      	clause.push(~Glucose::mkLit(l));
      } else if (solver.modelValue (l) == l_False) {
	      clause.push(Glucose::mkLit(l));
      }
    }
   
  private:
    std::unordered_map<size_t, Glucose::Lit>& alreadyCreated;
    std::unordered_map<Glucose::Var,Words::Constraints::Constraint_ptr>& constraints;
    std::unordered_map<Glucose::Var,Words::Equation>& eqs;
    std::unique_ptr<Words::Job>  job  = std::make_unique<Words::Job> ();
    Glucose::Solver& solver;
    Glucose::vec<Glucose::Lit> clause;
    std::unordered_map<size_t,ASTNode_ptr>& neqmap;
    Glucose::vec<Glucose::Lit>& assumptions;
  };
  
}
