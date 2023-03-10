#include "words/algorithms.hpp"

Words::Algorithms::ParikhImage Words::Algorithms::calculateParikh (Word& w){
  ParikhImage image;
  for (auto entry : w) {
	image[entry]++;
  }
  return image;
}

void Words::Algorithms::resetParikImage(ParikhImage& image){
  for (auto& x : image){
	x.second = 0;
  }
}

void Words::Algorithms::emptyParikhImage(Equation& eq, ParikhImage& image){
    for (auto a : eq.ctxt->getTerminalAlphabet()){
        image[a] = 0;
    }
    for (auto a : eq.ctxt->getVariableAlphabet()){
        image[a] = 0;
    }
}

void Words::Algorithms::emptyParikhMatrix(Equation& eq, ParikhMatrix& m){
    ParikhImage image;
    emptyParikhImage(eq,image);
    m.resize(1);
    m[0] = image;
}


void Words::Algorithms::calculateParikhMatrices (Word& w, ParikhMatrix& p_pm, ParikhMatrix& s_pm) {
  p_pm.resize (w.characters());
  s_pm.resize (w.characters());
  p_pm.back() = calculateParikh(w);
  ParikhImage s_start = p_pm.back ();
  resetParikImage(s_start);
  s_pm[0] = s_start;
  auto it = w.rbegin();
  auto end = w.rend();
  const int wSize = w.characters()-1;
  int iSize = w.characters()-1;
  for (; it != end && iSize > 0; ++it) {
	ParikhImage p_nPi = p_pm[iSize];
    ParikhImage s_nPi = s_pm[wSize-iSize];
    s_nPi[*it]++;
    s_pm[wSize-iSize] = s_nPi;
    iSize--;
    p_nPi[*it]--;
	p_pm[iSize] = p_nPi;    
  }
}
