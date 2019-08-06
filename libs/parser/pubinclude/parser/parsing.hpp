#ifndef _PARSING__
#define _PARSING__
#include <istream>
#include "solvers/solvers.hpp"

namespace Words {
  class Options;
  class ParserInterface {
  public:
	virtual ~ParserInterface () {}
	virtual Solvers::Solver_ptr Parse (Words::Options& opt,std::ostream& err) = 0;
  };

  using Parser_ptr = std::unique_ptr<ParserInterface>;
  
  enum class ParserType {
					 Standard,
					 SMTParser
  };

  Parser_ptr makeParser (ParserType,std::istream&);
  
}


#endif 
