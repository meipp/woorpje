#ifndef _ALGORITHMS__
#define _ALGORITHMS__

#include <unordered_map>
#include "words/words.hpP"

namespace Words {
  namespace Algorithms {
	using ParikhImage = std::unordered_map<IEntry*,int64_t>;

	ParikhImage calculateParikh (Words::Equation& eq) {
	  ParikhImage image;
	  for (auto entry : eq.lhs) {
		image[entry]++;
	  }
	  for (auto entry : eq.rhs) {
		image[entry]--;
	  }
	  return image;
	}

	
	
  }
}

#endif
