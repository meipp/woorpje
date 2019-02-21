#ifndef _WORD_EXCEPTIONS__
#define _WORD_EXCEPTIONS__
#include <stdexcept>

namespace Words {
  class WordException : public std::runtime_error  {
  public:
	WordException (const std::string& str) : runtime_error (str) {}
  };
}


#endif
