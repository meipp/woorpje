#ifndef _CONSTRAINTS__
#define _CONSTRAINTS__

#include <memory>
#include <ostream>

namespace Words  {
  namespace Constraints {
	class LinearConstraint;
	class Constraint {
	public:
	  virtual bool isLinear () {return false;}
	  virtual LinearConstraint* getLinconstraint () {return nullptr;}
	  virtual std::ostream& output (std::ostream&) const = 0;
	};

	using Constraint_ptr = std::unique_ptr<Constraint>;

	inline std::ostream& operator<< (std::ostream& os, const Constraint& c) {
	  return c.output(os);
	}
	
  }
}

#endif
