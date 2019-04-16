#ifndef _ALGORITHMS__
#define _ALGORITHMS__

#include <unordered_map>
#include "words/words.hpp"

namespace Words {
  namespace Algorithms {
	using ParikhImage = std::unordered_map<IEntry*,int64_t>;
	using ParikhMatrix = std::vector<ParikhImage>;

	ParikhImage calculateParikh (Word& w) {
	  ParikhImage image;
	  for (auto entry : w) {
		image[entry]++;
	  }
	  return image;
	}

	void resetParikImage(ParikhImage& image){
		for (auto x : image){
			image[x.first] = 0;
		}
	}

	void calculateParikhMatrices (Word& w, ParikhMatrix& p_pm, ParikhMatrix& s_pm) {
		p_pm[w.characters()-1] = calculateParikh(w);
		ParikhImage* s_start = p_pm[w.characters()-1];
		resetParikImage(*s_start);
		s_pm[0] = *s_start;
		auto itBegin = w.begin();
		auto it = w.end()-1;
		int wSize = w.characters()-1;
		int iSize = w.characters()-1;
		for (; it != itBegin; --it) {
			ParikhImage *p_nPi = p_pm[iSize];
			ParikhImage *s_nPi = p_pm[wSize-iSize];
			iSize--;
			p_nPi[*it]--;
			s_nPi[*it]++;
			p_pm[iSize] = p_nPi;
			p_pm[wSize-iSize] = s_nPi;
		}
	}



  }
}

#endif
