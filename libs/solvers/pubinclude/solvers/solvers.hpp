#ifndef _SOLVERS__
#define _SOLVERS__

#include <ostream>
#include <memory>
#include <boost/format.hpp>
#include <chrono>

#include "words/words.hpp"
#include "solvers/timing.hpp"

namespace Words {
  namespace Solvers {
	enum class Types {
					  SatEncoding,
					  SatEncodingOld,
					  Reachability,
					  PureSMT,
					  Levis
	};

	enum class Result {
					   HasSolution,
					   NoSolution,
					   DefinitelyNoSolution,
					   NoIdea
	};


	namespace Levis {
	  void selectVariableTerminalRatio (double d);
	  void selectWaitingListReached (size_t);
	  void selectCalculateTotalEquationSystemSize (double d);
	  void selectEquationLengthExceeded (size_t);
	  void selectNone();

	  enum class SearchOrder{
				 DepthFirst,
				 BreadthFirst
	  };
	  
	  template<SearchOrder,class ...Args>
	  void setSearchOrder (Args...);
	  
	}
	
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
	  virtual void progressMessage (const std::string& s) = 0;
	};

	class StreamRelay :public MessageRelay {
	public:
	  StreamRelay (std::ostream& os) : os(os) {
		last = std::chrono::high_resolution_clock::now();
	  }
	  void progressMessage (const std::string& s) override {
		timep now = std::chrono::high_resolution_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>( now - last ).count() >= 1000 ) {
		  os << "\x1b[2K" << arr[i++] << " " << s << "\r";
		  if (arr[i] == '\0')
			i = 0;
		  last = now;
		  os.flush();
		}
	   
	  }
	  void pushMessage (const std::string& s) override{os << s << std::endl;}
	private:
	  using timep = std::chrono::high_resolution_clock::time_point;
	  std::ostream& os;
	  const char* arr= "/-\\|"; 
      size_t i = 0;
	  timep last;
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
	  virtual void setSubstitution (Words::Substitution&) override {}
	  virtual void diagnosticString (const std::string&) override {}
	  virtual void timingInfo (const Timing::Keeper&) override {} 
	};
	
	class Solver {
	public:
	  virtual Result Solve (Words::Options&,MessageRelay&) = 0;

	  //Should only be called if Result returned HasSolution
	  virtual void getResults (ResultGatherer& r) = 0;
      virtual void getMoreInformation (std::ostream&){};
	  virtual void enableDiagnosticOutput () {}

	  virtual ~Solver () = default;
	};

	using Solver_ptr =std::unique_ptr<Solver>;
	
	template<Types,typename ...Args>
	Solver_ptr makeSolver (Args... args);
  }
}


#endif
