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
  void setSubstitution (Words::Substitution& w) {
	std::cout << "Substitution: " << std::endl;
	for (auto& it : w) {
	  std::cout << it.first->getRepr () << ": ";
	  for (auto c : it.second) {
		std::cout << c->getRepr ();
	  }
	  std::cout << std::endl;
	}
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
  }
  
};

void printContactDetails (std::ostream& os) {
  os << Version::ORGANISATION << std::endl;
  os << Version::MAINDEVELOPERNAME <<  "<" << Version::MAINDEVELOPEREMAIL << ">" << std::endl;
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
	("nobanner,n",po::bool_switch(&suppressbanner), "Suppress the panner.")
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
  try {
	std::fstream inp;
	inp.open (conffile);
	auto parser = Words::Parser::Create (inp);
	parsesucc = parser->Parse (opt,std::cout);
	inp.close ();
  }catch (Words::WordException& e) {
	std::cerr << e.what () << std::endl;
	return -1;
  }

  if (parsesucc) {
	auto solver = Words::Solvers::makeSolver<Words::Solvers::Types::SatEncoding> ();
	if (diagnostic)
	  solver->enableDiagnosticOutput ();
	Words::Solvers::StreamRelay relay (std::cout);
	auto ret =  solver->Solve (opt,relay);
	if (ret == Words::Solvers::Result::HasSolution) {
	  std::cout << "Got Solution " << std::endl;
	  CoutResultGatherer gatherer;
	  solver->getResults (gatherer);
	  return 0;
	}
	else if ( ret == Words::Solvers::Result::NoSolution) {
	  std::cout << "No solution" << std::endl;;
	  return 20;
	}
	
	else {
	  std::cout << "No Idea" << std::endl;;
	  return 10;
	}
  }
  std::cout << "Parse error" << std::endl;
}
