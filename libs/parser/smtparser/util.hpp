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

  // lhs <-> \/ rhs
  void reify_or_bi(Glucose::Solver & s, Glucose::Lit lhs, Glucose::vec<Glucose::Lit> & rhs){
    reify_or (s,lhs,rhs);
    for(int i = 0 ; i < rhs.size();i++){
      Glucose::vec<Glucose::Lit> ps;
      ps.push(~rhs[i]);
      ps.push(lhs);
      s.addClause(ps);
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


  
  
}
