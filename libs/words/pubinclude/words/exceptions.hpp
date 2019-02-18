#include <stdexcept>

namespace Words {
  class WordException : public std::runtime_error  {
  public:
	WordException (const std::string& str) : runtime_error (str) {}
  };
}
