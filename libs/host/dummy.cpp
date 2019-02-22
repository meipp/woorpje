#include <cstdint>
#include <ostream>
#include <iostream>
#include <csignal>
#include <sys/resource.h>

#include "host/exitcodes.hpp"

namespace Words {
  namespace Host {


	
	bool setCPULimit (size_t lim, std::ostream& os) {
	  os << "Failed setting CPU-limit" <<std::endl;
	  return false;
	}

	bool setVMLimit (size_t mbytes,  std::ostream& os) {
	  os << "Failed setting VM-limit" <<std::endl;
	  return false;
	}
	
  }
}
