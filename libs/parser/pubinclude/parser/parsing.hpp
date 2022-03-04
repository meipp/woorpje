#ifndef _PARSING__
#define _PARSING__
#include <istream>
#include "words/words.hpp"
#include "solvers/solvers.hpp"

namespace Words {

  struct Job {
	Words::Options options;
	Solvers::Solver_ptr solver;
  };
  
  class JobGenerator {
  public:
	virtual std::unique_ptr<Job> newJob ()  = 0;
  virtual ~JobGenerator() = default;
  };
  
  class ParserInterface {
  public:
	virtual ~ParserInterface () {}
	virtual std::unique_ptr<JobGenerator> Parse (std::ostream& err) = 0;
  };

  using Parser_ptr = std::unique_ptr<ParserInterface>;
  
  enum class ParserType {
					 Standard,
					 SMTParser
  };

  Parser_ptr makeParser (ParserType,std::istream&);
  
}


#endif 
