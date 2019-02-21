#ifndef _SOLVER_EXECPT__
#define _SOLVER_EXECPT__

#include "words/exceptions.hpp"

namespace Words {
  namespace Solvers {
	class OutOfMemoryException : public Words::WordException {
	public:
	  OutOfMemoryException () : WordException ("Out of memory") {}
	};
  }
}

#endif
