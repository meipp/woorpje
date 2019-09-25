#include "parser/parsing.hpp"

namespace Words {

  Parser_ptr makeStdParser (std::istream&);
  Parser_ptr makeSMTParser (std::istream&);
  

  Parser_ptr makeParser (ParserType t,std::istream& is) {
#ifdef SMTPARSER
    return makeSMTParser (is);
#endif
    return makeStdParser (is);
  }
}
