#ifndef _RESORUCES__
#define _RESORUCES__

#include <ostream>

namespace Words {
  namespace Host {
	bool setVMLimit (size_t, std::ostream&);
	bool setCPULimit (size_t, std::ostream&);
  }
}

#endif
