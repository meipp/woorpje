#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/program_options.hpp>

#include "words/words.hpp"
#include "words/exceptions.hpp"
#include "parser/parser.hpp"
#include "solvers/solvers.hpp"

#include "config.h"

namespace po = boost::program_options;

class CoutResultGatherer : public Words::Solvers::DummyResultGatherer {
public:
  CoutResultGatherer (std::vector<Words::Equation>& eqs) : eqs(eqs) {}
  void setSubstitution (Words::Substitution& w) {
	std::cout << "Substitution: " << std::endl;
	for (auto& it : w) {
	  std::cout << it.first->getRepr () << ": ";
	  for (auto c : it.second) {
		std::cout << c->getRepr ();
	  }
	  std::cout << std::endl;
	}
	std::cout << std::endl;

	std::cout << "Equations after substition" <<std::endl;
	for (auto& eq : eqs) {
	  printWordWithSubstitution (eq.lhs,w);
	  std::cout << " == ";
	  printWordWithSubstitution (eq.rhs,w);
	  std::cout <<std::endl;
	}
	std::cout << std::endl;
  }

  void diagnosticString (const std::string& s) override {
	std::cout << "Diagnostic from Solvers" << std::endl;
	std::cout << s << std::endl;
  }

  void timingInfo (const Words::Solvers::Timing::Keeper& keep) override {
	std::cout << "Timing info" << std::endl;
	for (const auto& entry : keep) {
	  std::cout << entry.name << " : " << entry.time << " milliseconds" <<std::endl;
	}
	std::cout << std::endl;
  }
  
private:  
  void printWordWithSubstitution (Words::Word& word, Words::Substitution& w) {
	for (auto c : word) {
	  if (c->isTerminal ())
		std::cout << c->getRepr ();
	  else {
		auto sub = w[c];
		for (auto s : sub) {
		  std::cout << s->getRepr ();
		}
	  }
	}
  }
  
  std::vector<Words::Equation> eqs;
};

void printContactDetails (std::ostream& os) {
  os << Version::ORGANISATION << std::endl;
  os << Version::MAINDEVELOPERNAME <<  " <" << Version::MAINDEVELOPEREMAIL << ">" << std::endl;
}


void printBanner (std::ostream& os) {
  os << Version::TOOLNAME << " "<<  Version::VERSION_MAJOR << "." << Version::VERSION_MINOR << " (" << __DATE__  << ")" <<std::endl;
  os << "Revision: " << Version::GIT_HASH << std::endl;
  os << Version::DEPSACK << std::endl;
  printContactDetails (os);
  os << std::endl;
}


void printHelp (std::ostream& os,po::options_description& desc) {
  printBanner (os);
  os << desc;
}

int main (int argc, char** argv) {
  bool diagnostic = false;
  bool suppressbanner = false;
  bool help = false;
  std::string conffile;
  po::options_description desc("General Options");
  desc.add_options ()
	("help,h",po::bool_switch(&help), "Help message.")
	("nobanner,n",po::bool_switch(&suppressbanner), "Suppress the banner.")
	("diagnostics,d",po::bool_switch(&diagnostic), "Enable Diagnostic Data.")
	("configuration,c",po::value<std::string>(&conffile), "Configuration file");
  
  po::positional_options_description positionalOptions; 
  positionalOptions.add("configuration", 1); 
  po::variables_map vm; 
  try {
	po::store(po::command_line_parser(argc, argv).options(desc) 
			  .positional(positionalOptions).run(), vm);
	po::notify (vm);
   
  }
  catch(po::error& e) {
	if (help)
	  printHelp (std::cout,desc);
	else
	  std::cerr << e.what();
	return -1;
  }
  if (help) {
	printHelp (std::cout,desc);
	return -1;
  }
  if (!suppressbanner)
	printBanner (std::cout);
  if (conffile == "" ) {
	std::cerr << "Configuration file not specified" << std::endl;
	return -1;
  }
  Words::Options opt;
  bool parsesucc = false;
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
	if (diagnostic)
	  solver->enableDiagnosticOutput ();
	Words::Solvers::StreamRelay relay (std::cout);
	auto ret =  solver->Solve (opt,relay);
	switch (ret) {
	  
	  
	case Words::Solvers::Result::HasSolution: {
	  std::cout << "Got Solution " << std::endl << std::endl;;
	  CoutResultGatherer gatherer (opt.equations);
	  solver->getResults (gatherer);
	  return 0;
	}
	case Words::Solvers::Result::NoSolution: {
	  std::cout << "No solution" << std::endl;;
	  return 20;
	}
	  
	default:
	  std::cout << "No Idea" << std::endl;;
	  return 10;
	}
  }
  else 
	std::cout << "Parse error" << std::endl;
}
