#ifndef _WORD_EXCEPTIONS__
#define _WORD_EXCEPTIONS__
#include <stdexcept>
#include <string>

namespace Words {
  class WordException : public std::runtime_error  {
  public:
	WordException (const std::string& str) : runtime_error (str) {}
  };
  
  class ConstraintUnsupported : public WordException  {
  public:
    ConstraintUnsupported () : WordException ("A constraint is unsupported for the solver") {}
  };

  class UnsupportedFeature : public WordException  {
  public:
	UnsupportedFeature () : WordException ("A feature is unsupported by Woorpje") {}
  UnsupportedFeature(const std::string& str) : WordException (str) {}
  };

  class UsingEpsilonAsNonEpsilon : public WordException  {
  public:
    UsingEpsilonAsNonEpsilon () : WordException ("You are using epsilons as a non-epsilon character") {}
  };

}


#endif
