#include "parser/parsing.hpp"


namespace Words {

  Parser_ptr makeStdParser (std::istream&);
  Parser_ptr makeSMTParser (std::istream&);
  

  Parser_ptr makeParser (ParserType, std::istream& is) {
#ifdef SMTPARSER
      return makeSMTParser (is);
#else
      return makeStdParser (is);
#endif
  }
}
