#ifndef _ALGORITHMS__
#define _ALGORITHMS__

#include <unordered_map>
#include "words/words.hpp"

namespace Words {
  namespace Algorithms {
	using ParikhImage = std::unordered_map<IEntry*,int64_t>;
	using ParikhMatrix = std::vector<ParikhImage>;
	
	ParikhImage calculateParikh (Word& w);

	void resetParikImage(ParikhImage& image);
	
	void calculateParikhMatrices (Word& w, ParikhMatrix& p_pm, ParikhMatrix& s_pm);



  }
}

#endif
