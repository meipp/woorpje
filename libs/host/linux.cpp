#include <cstdint>
#include <ostream>
#include <iostream>
#include <csignal>
#include <sys/resource.h>

#include "host/exitcodes.hpp"

namespace Words {
  namespace Host {

	void cpu_limit_reached (int) {
	  Terminate (ExitCode::TimeOut,std::cerr);
	}
	
	bool setCPULimit (size_t lim, std::ostream& os) {
	  rlimit limit;
	  if (!getrlimit (RLIMIT_CPU,&limit)) {
		if (limit.rlim_max == RLIM_INFINITY ) {
		  limit.rlim_cur = static_cast<rlim_t> (lim);
		  if (setrlimit(RLIMIT_CPU, &limit) == 0) {
			os << "Set CPU-limit to " << static_cast<rlim_t> (lim)  << " second(s)" <<std::endl;
			std::signal (SIGXCPU,cpu_limit_reached);
			return true;
		  }
		}
	  }
	  os << "Failed setting CPU-limit" <<std::endl;
	  return false;
	  
	}

	bool setVMLimit (size_t mbytes,  std::ostream& os) {
	  size_t limit_vm = mbytes*1024*1024;
	  rlimit limit;
	  if (!getrlimit (RLIMIT_AS,&limit)) {
		if (limit.rlim_max == RLIM_INFINITY ) {
		  limit.rlim_cur = static_cast<rlim_t> (limit_vm);
		  if (setrlimit(RLIMIT_AS, &limit) == 0) {
			os << "Set VM-limit to " << mbytes  << " MB" <<std::endl;
			return true;
		  }
		}
	  }
	  
	  os << "Failed setting VM-limit" <<std::endl;
	  return false;
	}
	
  }
}
