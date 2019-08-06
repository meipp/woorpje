#include "parser/parsing.hpp"

namespace Words {

  Parser_ptr makeStdParser (std::istream&);
  Parser_ptr makeSMTParser (std::istream&);
  

  Parser_ptr makeParser (ParserType t,std::istream& is) {
#if defined(SMTPARSER)
	return makeSMTParser (is);
#else
	return makeStdParser (is);
#endif
  }
}
