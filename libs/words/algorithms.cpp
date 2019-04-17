#include "words/algorithms.hpp"

Words::Algorithms::ParikhImage Words::Algorithms::calculateParikh (Word& w){
  ParikhImage image;
  for (auto entry : w) {
	image[entry]++;
  }
  return image;
}

void Words::Algorithms::resetParikImage(ParikhImage& image){
  for (auto x : image){
	image[x.first] = 0;
  }
}

void Words::Algorithms::calculateParikhMatrices (Word& w, ParikhMatrix& p_pm, ParikhMatrix& s_pm) {
  p_pm.resize (w.characters());
  s_pm.resize (w.characters());
  p_pm.back() = calculateParikh(w);
  ParikhImage s_start = p_pm.back ();
  resetParikImage(s_start);
  s_pm[0] = s_start;
  auto itBegin = w.begin();
  auto it = w.end()-1;
  int wSize = w.characters()-1;
  int iSize = w.characters()-1;
  for (; it != itBegin; --it) {
	ParikhImage p_nPi = p_pm[iSize];
	ParikhImage s_nPi = s_pm[wSize-iSize];
	iSize--;
	p_nPi[*it]--;
	s_nPi[*it]++;
	p_pm[iSize] = p_nPi;
	s_pm[wSize-iSize] = s_nPi;
  }
}
