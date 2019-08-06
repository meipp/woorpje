#include "parser/parsing.hpp"

namespace Words {

  Parser_ptr makeStdParser (std::istream&);
  

  Parser_ptr makeParser (ParserType t,std::istream& is) {
	switch (t) {
	case ParserType::Standard:
	  return makeStdParser (is);
	default:
	  return nullptr;
	}

  }
}
