#ifndef _SOLVERS__
#define _SOLVERS__

#include <ostream>
#include <memory>
#include <boost/format.hpp>

#include "words/words.hpp"
#include "solvers/timing.hpp"

namespace Words {
  namespace Solvers {
	enum class Types {
					  SatEncoding,
					  SatEncodingOld
	};

	enum class Result {
					   HasSolution,
					   NoSolution,
					   NoIdea
	};

	class Formatter {
	public:
	  Formatter (const std::string& str) : formatter(str) {}
	  template<typename T> 
	  Formatter& operator% (const T& t) {
		formatter % t;
		return *this;
	  }
	  
	  const std::string str () {return formatter.str();}
	  
	private:
	  boost::format formatter;
	};
	
	class MessageRelay {
	public:
	  virtual void pushMessage (const std::string& s) = 0;
	};

	class StreamRelay :public MessageRelay {
	public:
	  StreamRelay (std::ostream& os) : os(os) {}
	  void pushMessage (const std::string& s) {os << s << std::endl;}
	private:
	  std::ostream& os;
	};

	
	
	//Add Callback function for solver to provide solution details
	class ResultGatherer {
	public:
	  virtual void setSubstitution (Words::Substitution&) = 0;
	  virtual void diagnosticString (const std::string& s) = 0;
	  virtual void timingInfo (const Timing::Keeper& s) = 0;
	};
	
	class DummyResultGatherer : public ResultGatherer {
	public:
	  virtual void setSubstitution (Words::Substitution&) {}
	  virtual void diagnosticString (const std::string&) {}
	  virtual void timingInfo (const Timing::Keeper&) {} 
	};
	
	class Solver {
	public:
	  virtual Result Solve (Words::Options&,MessageRelay&) = 0;

	  //Should only be called if Result returned HasSolution
	  virtual void getResults (ResultGatherer& r) = 0;
	  virtual void enableDiagnosticOutput () {}
	};

	using Solver_ptr =std::unique_ptr<Solver>;
	
	template<Types,typename ...Args>
	Solver_ptr makeSolver (Args... args);
  }
}


#endif
