#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/program_options.hpp>

#include "words/words.hpp"
#include "words/exceptions.hpp"
#include "host/resources.hpp"
#include "host/exitcodes.hpp"
#include "parser/parser.hpp"
#include "solvers/solvers.hpp"
#include "solvers/exceptions.hpp"

#include "config.h"

namespace po = boost::program_options;

class CoutResultGatherer : public Words::Solvers::DummyResultGatherer {
public:
  CoutResultGatherer (const Words::Options& opt) : opt(opt) {}
  void setSubstitution (Words::Substitution& w) {
	std::cout << "Substitution: " << std::endl;
	for (size_t var = 0; var < opt.context.nbVars(); var++) {
	  auto variable = opt.context.getVariable (var);
	  std::cout << variable->getRepr () << ": ";
	  for (auto c : w[variable]) {
		std::cout << c->getRepr ();
	  }
	  std::cout << std::endl;
	}
	std::cout << std::endl;
	
	std::cout << "Equations after substition" <<std::endl;
	for (auto& eq : opt.equations) {
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
  void printWordWithSubstitution (const Words::Word& word, Words::Substitution& w) {
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
  
  const Words::Options& opt;
};

void printContactDetails (std::ostream& os) {
  os << Version::ORGANISATION << std::endl;
  os << Version::MAINDEVELOPERNAME <<  " - " << Version::MAINDEVELOPEREMAIL << "" << std::endl;
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
  size_t cpulim = 0;
  size_t vmlim = 0;
  std::string conffile;
  po::options_description desc("General Options");
  desc.add_options ()
	("help,h",po::bool_switch(&help), "Help message.")
	("nobanner,n",po::bool_switch(&suppressbanner), "Suppress the banner.")
	("diagnostics,d",po::bool_switch(&diagnostic), "Enable Diagnostic Data.")
	("configuration,c",po::value<std::string>(&conffile), "Configuration file")
	("cpulim,C",po::value<size_t>(&cpulim), "CPU Limit in seconds")
	("vmlim,V",po::value<size_t>(&vmlim), "VM Limit in MBytes");
  
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

  if(cpulim && !Words::Host::setCPULimit (cpulim,std::cerr))
	Words::Host::Terminate (Words::Host::ExitCode::ConfigurationError,std::cout);

  if(vmlim && !Words::Host::setVMLimit (vmlim,std::cerr))
	Words::Host::Terminate (Words::Host::ExitCode::ConfigurationError,std::cout);

  
  
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
	try {
	  Words::Solvers::StreamRelay relay (std::cout);
	  auto ret =  solver->Solve (opt,relay);
	  switch (ret) {
	  
	  
	  case Words::Solvers::Result::HasSolution: {
		CoutResultGatherer gatherer (opt);
		solver->getResults (gatherer);
		Words::Host::Terminate (Words::Host::ExitCode::GotSolution,std::cout);
	  }
	  case Words::Solvers::Result::NoSolution: {
		std::cout << "No solution due to set bounds" << std::endl;
		Words::Host::Terminate (Words::Host::ExitCode::NoSolution,std::cout);
	  }
	  case Words::Solvers::Result::DefinitelyNoSolution: {
	  		std::cout << "No solution" << std::endl;
	  		Words::Host::Terminate (Words::Host::ExitCode::DefinitelyNoSolution,std::cout);
	  }
		
	  default:
		Words::Host::Terminate (Words::Host::ExitCode::NoIdea,std::cout);
	  }
	}catch (Words::Solvers::OutOfMemoryException&) {
	  Words::Host::Terminate (Words::Host::ExitCode::OutOfMemory,std::cout);
	}
  }
  else {
	Words::Host::Terminate (Words::Host::ExitCode::ConfigurationError,std::cout);
  }
}
