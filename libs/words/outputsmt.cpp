#include <sstream>

#include "words/words.hpp"
#include "words/linconstraint.hpp"


namespace Words {
  void encodeVariable (std::ostream& os, const Words::IEntry* v) {
    os << "(declare-fun ";
    v->output(os) << "() String" << ")" << std::endl;
}

void encodePreamble (std::ostream& os, Words::Context& c) {
  os << "(set-logic QF_S)" << std::endl;
  auto vars = c.nbVars ();
  for (size_t i = 0; i <vars; i++) {
	encodeVariable (os,c.getVariable (i));
  }
}


void encodeEnd (std::ostream& os) {
  os << "(check-sat)" << std::endl;
}
void encodeWord (std::ostream& os, const Words::Word& w) {
  std::stringstream str;
  os << "(str.++ ";
  for (auto c : w) {
    if (c->isVariable ()) {
      if (str.str() != "") {
	os << " \"" << str.str() << "\" ";
	str.str("");
      }
      c->output (os)<< " ";
    }
    else {
      c->output(str);
    }
  }
  if (str.str() != "")
	os << " \"" << str.str() <<  "\"";
  os << " \"\")";
}

void encodeEquation (std::ostream& os, const Words::Equation& w) {
  os << "(assert (=";
  encodeWord (os,w.lhs);
  encodeWord (os,w.rhs);
  os << ") )" << std::endl; 
}

template<class T>
void encodeNumber (std::ostream& os, T t) {
  if (t < 0) {
	os << "(- " << abs(t) << ")";
  }
  else
	os << t;
}

void encodeMultiplicity (std::ostream& os, const Words::Constraints::VarMultiplicity& w) {
  os << "(* (str.len "; w.entry->output(os) << ")";
  encodeNumber (os,w.number);
  os << ")"; 
}



void encodeLinConstraint (std::ostream& os, const Words::Constraints::LinearConstraint* c) {
  os <<"(assert  (<= ";
  auto it = c->begin ();
  while (it != c->end()) {
	std::stringstream str;
	encodeMultiplicity (str,*it);
	++it;
	if (it == c->end()) {
	  os << str.str();
	}
	else {
	  os << "(+ " << str.str() << " ";
	}
  }

  it = c->begin ();
  while (it != c->end()) {
	++it;
	if (it != c->end ())
	  os << ")";
  }

  os << " ";
  encodeNumber(os,c->getRHS ());
  os << ") )";
  os << std::endl;
  /*encodeMultiplicity (str,cc);
  strs.push_back (str.str());
  str.str("");
  }
  */
  
  
}
  
  void outputSMT (std::ostream& os, const Words::Options& opt) {
    encodePreamble (os,*opt.context);
    for (auto& eq : opt.equations)
      encodeEquation (os,eq);
    for (auto& eq : opt.constraints) {
      if (eq->isLinear ()) {
	encodeLinConstraint (os,eq->getLinconstraint ());
      }
      else {
	std::cerr << "Encountered non-lineary constraint" << std::endl;
      }
    }
    encodeEnd (os);
  }
}
