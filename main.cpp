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
#include "smt/smtsolvers.hpp"


#include "solvers/simplifiers.hpp"
#include "solvers/exceptions.hpp"


#include "config.h"

namespace po = boost::program_options;

struct LevisHeuristics {
  size_t which = 0;
  double varTerminalRatio = 1.1;
  size_t wlistLimit = 2;
  double growthFactor = 1.1;
  size_t eqLength = 100;
};

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

void setupLevis (LevisHeuristics& l) {
  using namespace Words::Solvers::Levis;
  switch (l.which) {
  case 0:
	selectVariableTerminalRatio(l.varTerminalRatio);
	break;
  case 1:
	selectWaitingListReached (l.wlistLimit);
	break;
  case 2:
    selectCalculateTotalEquationSystemSize(l.growthFactor);
    break;
  case 3:
    selectEquationLengthExceeded(l.eqLength);
    break;
  case 4:
    selectNone();
    break;
  }
}

void printHelp (std::ostream& os,po::options_description& desc) {
  printBanner (os);
  os << desc;
}

void setSMTSolver (size_t i) {
  switch (i) {
  case 0:
	Words::SMT::setSMTSolver (Words::SMT::SMTSolver::Z3);
	break;
  case 2:	
	Words::SMT::setSMTSolver (Words::SMT::SMTSolver::Z3Str3);
	break;
  case 1: 
  default:
	Words::SMT::setSMTSolver (Words::SMT::SMTSolver::CVC4);
	break;
  }
}

Words::Solvers::Solver_ptr buildSolver (size_t i)  {
  switch (i) {
  case 0:
	return nullptr;
	break;
  case 1:
	return Words::Solvers::makeSolver<Words::Solvers::Types::SatEncoding> (static_cast<size_t> (0));
  case 2:
	return Words::Solvers::makeSolver<Words::Solvers::Types::SatEncodingOld> (static_cast<size_t> (0));
  case 3:
	return Words::Solvers::makeSolver<Words::Solvers::Types::PureSMT> ();
  case 4:
	return Words::Solvers::makeSolver<Words::Solvers::Types::Levis> ();
  }
  return nullptr;
}


int main (int argc, char** argv) {
  bool diagnostic = false;
  bool suppressbanner = false;
  bool help = false;
  bool simplifier = false;
  size_t cpulim = 0;
  size_t vmlim = 0;
  size_t solverr  = 0;
  std::string conffile;
  po::options_description desc("General Options");
  LevisHeuristics lheu;
  desc.add_options ()
	("help,h",po::bool_switch(&help), "Help message.")
	("nobanner,n",po::bool_switch(&suppressbanner), "Suppress the banner.")
	("diagnostics,d",po::bool_switch(&diagnostic), "Enable Diagnostic Data.")
	("configuration,c",po::value<std::string>(&conffile), "Configuration file")
	("cpulim,C",po::value<size_t>(&cpulim), "CPU Limit in seconds")
	("simplify",po::bool_switch(&simplifier), "Enable simplifications")
	("vmlim,V",po::value<size_t>(&vmlim), "VM Limit in MBytes")
	("solver",po::value<size_t>(&solverr), "Solver Strategy\n"
								"\t  0 Defined in input file\n"
								"\t  1 Sat Encoding via Glucose\n"
								"\t  2 Old Sat Encoding via Glucose\n"
								"\t  3 SMT\n"
								"\t  4 Levis Lemmas\n"
	 );
  size_t smtsolver = 0;
  size_t smttimeout = 0;
  po::options_description smdesc("SMT Options");
  smdesc.add_options()
	("smtsolver,S",po::value<size_t> (&smtsolver), "SMT Solver\n"
	"\t 0 Z3\n"
	"\t 1 CVC4\n"
	"\t 2 Z3Str3\n"
	 )
	("smttimeout",po::value<size_t> (&smttimeout), "Set timeout for SMTSolver (ms)");

  po::options_description levdesc("LevisSMT Options");
  levdesc.add_options()
    ("levisheuristics",po::value<size_t> (&lheu.which), "Levi Heuristics\n"
	"\t 0 VariableTerminalRatio\n"
    "\t 1 WaitingListLimitReached\n"
    "\t 2 EquationGrowth\n"
    "\t 3 EquationLengthExceeded\n"
    "\t 4 None\n"
	 )
	("VarTerminalRation",po::value<double> (&lheu.varTerminalRatio), "Variable Terminal Ratio")
    ("WaitingLimit",po::value<size_t> (&lheu.wlistLimit), "WaitingListLimit")
    ("growth",po::value<double> (&lheu.growthFactor), "Equation Growth Ratio")
    ("eqLength",po::value<size_t> (&lheu.eqLength), "Equation Length");
	
  
  desc.add (smdesc);
  desc.add (levdesc);
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
  setupLevis (lheu);
  setSMTSolver (smtsolver);
  Words::SMT::setDefaultTimeout (smttimeout);
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
  Words::Options foroutput;
  bool parsesucc = false;
  Words::Solvers::Solver_ptr solver = buildSolver (solverr);
  try {
	std::fstream inp;
	inp.open (conffile);
	auto parser = Words::Parser::Create (inp);
	auto s = parser->Parse (opt,std::cout);
	if (!solver) {
	  std::cout << "Using Solver from input file." << std::endl;
	  std::swap (solver,s);
	  //solver = s;
	}
	else {
	  std::cout << "Using command line forced  solver." << std::endl;
	}
	foroutput = opt;
	inp.close ();
  }catch (Words::WordException& e) {
	std::cerr << e.what () << std::endl;
	return -1;
  }
  CoutResultGatherer gatherer (foroutput);
  if (solver) {
	if (simplifier) {
	  std::cout << "Running Simplifiers" << std::endl;
	  Words::Substitution sub;
	  auto res = Words::Solvers::CoreSimplifier::solverReduce (opt,sub);
	  std::cout << "Equation System after simplification" << std::endl << opt << std::endl;;
	  
	  switch (res) {
		case ::Words::Solvers::Simplified::JustReduced:
		  break;
		case ::Words::Solvers::Simplified::ReducedNsatis:
		  Words::Host::Terminate (Words::Host::ExitCode::DefinitelyNoSolution,std::cout);
		case ::Words::Solvers::Simplified::ReducedSatis:
		  gatherer.setSubstitution (sub);
		  Words::Host::Terminate (Words::Host::ExitCode::GotSolution,std::cout);
	  }
	  
	}
	
	if (diagnostic)
	  solver->enableDiagnosticOutput ();	
	try {
	  Words::Solvers::StreamRelay relay (std::cout);
      auto ret =  solver->Solve (opt,relay);

      solver->getMoreInformation(std::cout);

      std::cout << "\n" << std::endl;

	  switch (ret) {
		
	  case Words::Solvers::Result::HasSolution: {
		solver->getResults (gatherer);
		Words::Host::Terminate (Words::Host::ExitCode::GotSolution,std::cout);
	  }
	  case Words::Solvers::Result::NoSolution: {
		Words::Host::Terminate (Words::Host::ExitCode::NoSolution,std::cout);
	  }
	  case Words::Solvers::Result::DefinitelyNoSolution: {
		Words::Host::Terminate (Words::Host::ExitCode::DefinitelyNoSolution,std::cout);
	  }
		
	  default:
		Words::Host::Terminate (Words::Host::ExitCode::NoIdea,std::cout);
	  }
	}catch (Words::Solvers::OutOfMemoryException&) {
	  Words::Host::Terminate (Words::Host::ExitCode::OutOfMemory,std::cout);
	}
	catch (Words::WordException& o) {
	  std::cout << o.what () << std::endl;
	  Words::Host::Terminate (Words::Host::ExitCode::ConfigurationError,std::cout);
	}
	
  }
  else {
	Words::Host::Terminate (Words::Host::ExitCode::ConfigurationError,std::cout);
  }
}
