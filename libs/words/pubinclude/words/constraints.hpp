#ifndef _CONSTRAINTS__
#define _CONSTRAINTS__

#include <memory>
#include <ostream>

namespace Words  {
  namespace Constraints {
	class LinearConstraint;
	class Unrestricted;
	class Constraint {
	public:
	  virtual ~Constraint() = default;
	  virtual bool isLinear () const  {return false;}
	  virtual bool isUnrestricted () const  {return false;}
	  virtual const LinearConstraint* getLinconstraint () const {return nullptr;}
	  virtual const Unrestricted* getUnrestricted () const {return nullptr;}
	  virtual std::ostream& output (std::ostream&) const = 0;
	  virtual uint32_t hash (uint32_t seed) const = 0;
	  virtual std::shared_ptr<Constraint> copy() const = 0;
	  virtual bool lhsEmpty() const {return false;}
	};
	
	using Constraint_ptr = std::shared_ptr<Constraint>;

	inline std::ostream& operator<< (std::ostream& os, const Constraint& c) {
	  return c.output(os);
	}
	
  }
}

#endif
