#ifndef _EXITCODES__
#define _EXITCODES__

#include <ostream>

namespace Words {
  namespace Host {
	enum class ExitCode {
						  GotSolution = 0,
						  NoSolution = 10,
						  DefinitelyNoSolution = 1,
						  NoIdea = 20,
						  OutOfMemory = 30,
						  TimeOut = 40,
						  ConfigurationError = 50
	};

	inline void Terminate (ExitCode e, std::ostream& os) {
	  switch (e) {
	  case ExitCode::GotSolution: 
		os << "Found a solution" << std::endl;
		  break;
	  case ExitCode::NoSolution:
		os << "Equation has no solution due to set bounds" << std::endl;
		break;
	  case ExitCode::DefinitelyNoSolution:
		os << "Equation has no solution" << std::endl;
		break;
	  case ExitCode::NoIdea:
		os << "Equation might have solution" << std::endl;
		break;
	  case ExitCode::OutOfMemory:
		os << "Ran out of memory" << std::endl;
		break;
	  case ExitCode::TimeOut:
		os << "Timed out" << std::endl;
		break;
	  default:
		;
	  }
	  std::exit(static_cast<int> (e) );
	}

  }
}

#endif
