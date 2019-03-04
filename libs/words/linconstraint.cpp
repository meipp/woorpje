#include <memory> 
#include <map>

#include "words/linconstraint.hpp"
#include "words/words.hpp"

namespace Words {
  namespace Constraints {
	template<Cmp outer>
	struct Helper{
	  Helper () : rhs(0)  {}
	  Constraint_ptr makeConstraint () { return nullptr;}
	  void add (IEntry* e, int64_t i) {
		multiplicities[e] += i;
	  }

	  void add (int64_t i) {
		rhs+= i;
	  }

	  std::vector<VarMultiplicity> makeMultiVector () {
		std::vector<VarMultiplicity> res;
		for (auto& mm : multiplicities) {
		  if (mm.second) {
			res.emplace_back (mm.first,mm.second);
		  }
		}
		return res;
	  }

	  std::vector<VarMultiplicity> makeNegMultiVector () {
		std::vector<VarMultiplicity> res;
		for (auto& mm : multiplicities) {
		  if (mm.second) {
			res.emplace_back (mm.first,-mm.second);
		  }
		}
		return res;
	  }
	  
	  std::map<IEntry*,int64_t> multiplicities;
	  int64_t rhs;
	};

	template<>
	Constraint_ptr Helper<Cmp::LEq>::makeConstraint () {
	  return std::make_unique<LinearConstraint> (makeMultiVector (),rhs);
	}

	template<>
	Constraint_ptr Helper<Cmp::Lt>::makeConstraint () {
	  return std::make_unique<LinearConstraint> (makeMultiVector (),rhs-1);
	}

	template<>
	Constraint_ptr Helper<Cmp::GEq>::makeConstraint () {
	  
	  return std::make_unique<LinearConstraint> (makeNegMultiVector (),-rhs);
	}

	template<>
	Constraint_ptr Helper<Cmp::Gt>::makeConstraint () {
	  return std::make_unique<LinearConstraint> (makeNegMultiVector (),-rhs-1);
	}


	template<Cmp c>
	LinConsBuilder<c>::LinConsBuilder () {
	  internal = std::make_unique<Helper<c>> ();
	}

	template<Cmp c>
	LinConsBuilder<c>::~LinConsBuilder () {}

	template<Cmp c>
	Constraint_ptr  LinConsBuilder<c>::makeConstraint () {
	  return internal->makeConstraint ();
	}
	
	template<Cmp c>
	void LinConsBuilder<c>::addLHS (IEntry* e, int64_t v) {
	  internal->add (e,v);
	}

	template<Cmp c>
	void LinConsBuilder<c>::addLHS (int64_t v) {
	  internal->add (-v);
	}

	template<Cmp c>
	void LinConsBuilder<c>::addRHS (IEntry* e, int64_t v) {
	  internal->add (e,-v);
	}

	template<Cmp c>
	void LinConsBuilder<c>::addRHS (int64_t v) {
	  internal->add (v);
	}

	template
	class LinConsBuilder<Cmp::Lt>;

	template
	class LinConsBuilder<Cmp::LEq>;

	
	template
	class LinConsBuilder<Cmp::GEq>;

		
	template
	class LinConsBuilder<Cmp::Gt>;
	 
	
  }
}
