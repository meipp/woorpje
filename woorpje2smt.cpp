#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <boost/program_options.hpp>

#include "words/words.hpp"
#include "words/exceptions.hpp"
#include "host/resources.hpp"
#include "host/exitcodes.hpp"
#include "parser/parser.hpp"
#include "solvers/solvers.hpp"
#include "solvers/exceptions.hpp"
#include "words/linconstraint.hpp"

void encodeVariable (std::ostream& os, const Words::IEntry* v) {
  os << "(declare-fun " << v->getRepr () << "() String" << ")" << std::endl;
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
	  os << c->getRepr ()<< " ";
	}
	else {
	  str << c->getRepr ();
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
  os << "(* (str.len " << w.entry->getRepr () << ")";
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

namespace po = boost::program_options;

int main (int argc, char** argv) {
  
  std::string conffile;
  std::string outputfile = "";
  po::options_description desc("General Options");
  desc.add_options ()
	("configuration,c",po::value<std::string>(&conffile), "Configuration file")
	("outputfile",po::value<std::string>(&outputfile), "Output file");
  
  po::positional_options_description positionalOptions; 
  positionalOptions.add("configuration", 1);
  positionalOptions.add("outputfile", 2);
  
  po::variables_map vm; 
  try {
	po::store(po::command_line_parser(argc, argv).options(desc) 
			  .positional(positionalOptions).run(), vm);
	po::notify (vm);
	
  }
  catch(po::error& e) {
	std::cerr << e.what () << std::endl;
	return -1;
  }
  
  
  Words::Options opt;
  Words::Solvers::Solver_ptr solver = nullptr;
  try {
	std::fstream inp;
	inp.open (conffile);
	auto parser = Words::Parser::Create (inp);
	solver = parser->Parse (opt,std::cout);
	inp.close ();
  }catch (Words::WordException& e) {
	std::cerr << e.what () << std::endl;
	return -1;
  }
  
  if (solver) {
	std::ostream* out = &std::cout;
	std::fstream outp;
	if (outputfile != "") {
	  out = &outp;
	  outp.open (outputfile.c_str(),std::fstream::out);
	}
	
	encodePreamble (*out,*opt.context);
	for (auto& eq : opt.equations)
	  encodeEquation (*out,eq);
	for (auto& eq : opt.constraints) {
	  if (eq->isLinear ()) {
		encodeLinConstraint (*out,eq->getLinconstraint ());
	  }
	  else {
		std::cerr << "Encountered non-lineary constraint" << std::endl;
	  }
	}
	encodeEnd (*out);
  }
  else {
	std::cerr << "Configuration Error" << std::endl;
  }
  
}

