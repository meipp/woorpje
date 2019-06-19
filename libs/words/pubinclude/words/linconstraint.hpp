#ifndef _LINCONSTRAINT__
#define _LINCONSTRAINT__

#include "words/words.hpp"
#include "words/constraints.hpp"
#include "host/hash.hpp"

namespace Words {
  namespace Constraints {
	struct VarMultiplicity {
	  VarMultiplicity (IEntry* e, int64_t n) : entry(e), number(n) {}
	  IEntry* entry;
	  int64_t number;
	};

	//Representation of linear constraints of the form:
	//n1*X1+n2*X2 ... nnXn <= k
	//where n1 .. nn and k are integers and X1...Xn are variables
	class LinearConstraint :public Constraint  {
	public:
	  LinearConstraint (std::vector<VarMultiplicity>&& vec, int64_t rhs) : variables(vec),rhs(rhs) {}  
	  auto begin () const {return variables.begin();}
	  auto end () const {return variables.end();}
	  const auto& getRHS () const {return rhs;}
	  std::ostream& output (std::ostream& o) const override {
		o << this << ": ";
		for  (auto& e : variables) {
		  o << e.number   <<"*"<<  e.entry->getRepr() << " "; 
		}
		o << " <= " << rhs;
		return o;
	  }
	  virtual bool isLinear () const override {return true;}
	  virtual const LinearConstraint* getLinconstraint () const override {return this;}
	  uint32_t hash (uint32_t seed) const override {
		seed = Words::Hash::Hash<VarMultiplicity> (variables.data(),variables.size(),seed);
		return Words::Hash::Hash<int64_t> (&rhs,1,seed);
	  }
		
	  virtual std::shared_ptr<Constraint> copy() const override {return std::make_shared<LinearConstraint>(*this);}
	  virtual bool lhsEmpty() const override {return variables.size() == 0;}
	
	private: 
	  std::vector<VarMultiplicity> variables;
	  int64_t rhs;
	};
	
	enum class Cmp{
				   Lt,
				   LEq,
				   GEq,
				   Gt
	};


	class LinearConstraintBuilder {
	public:
	  virtual Constraint_ptr  makeConstraint () = 0;
	  virtual void addLHS (IEntry* e, int64_t) = 0;
	  virtual void addLHS (int64_t ) = 0;
	  
	  virtual void addRHS (IEntry* e, int64_t) = 0;
	  virtual void addRHS (int64_t ) = 0;
	};

	

	template<Cmp d>
	class Helper;

	template<Cmp cc> 
	class LinConsBuilder : public LinearConstraintBuilder {
	public:
	  LinConsBuilder ();
	  ~LinConsBuilder ();
	  Constraint_ptr  makeConstraint ();
	  void addLHS (IEntry* e, int64_t);
	  void addLHS (int64_t );
	  
	  void addRHS (IEntry* e, int64_t);
	  void addRHS (int64_t );
	private:
	  
	  std::unique_ptr<Helper<cc> > internal;
	  
	};
	
	inline std::unique_ptr<LinearConstraintBuilder> makeLinConstraintBuilder (Words::Constraints::Cmp cmp) {
	  switch (cmp) {
	  case Words::Constraints::Cmp::Lt:
		return std::make_unique<LinConsBuilder<Cmp::Lt>> ();
	  case Words::Constraints::Cmp::LEq:
		return std::make_unique<LinConsBuilder<Cmp::LEq>> ();
	  case Words::Constraints::Cmp::Gt:
		return std::make_unique<LinConsBuilder<Cmp::Gt>> ();
	  case Words::Constraints::Cmp::GEq:
		return std::make_unique<LinConsBuilder<Cmp::GEq>> ();
	  default:
		return nullptr;
	  }
	  
	}
	
	
  }
}


#endif
