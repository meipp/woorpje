#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <boost/program_options.hpp>

#include "words/words.hpp"
#include "words/exceptions.hpp"
#include "host/resources.hpp"
#include "host/exitcodes.hpp"
#include "parser/parsing.hpp"
#include "solvers/solvers.hpp"
#include "solvers/exceptions.hpp"
#include "words/linconstraint.hpp"
#include "host/exitcodes.hpp"

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
  
  
  
  try {
	std::fstream inp;
	inp.open (conffile);
	auto parser = Words::makeParser (Words::ParserType::Standard,inp);
	auto jg =  parser->Parse (std::cout);
	inp.close ();
	size_t seg = 0;
	auto job = jg->newJob ();
	while (job) {
	  if (!job->options.hasIneqquality ()) {
	   
		Words::Options& opt = job->options;
		std::ostream* out = &std::cout;
		std::fstream outp;
		if (outputfile != "") {
		  std::stringstream str;
		  str << outputfile << "_" << ++seg;
		  outp.open (str.str().c_str(),std::fstream::out);
		  out = &outp;
	    
		}
		
		Words::outputSMT (*out,opt);
	  }
	  else {
	    std::cerr << "System has inequality...skipping" << std::endl;
	  }
	  job = jg->newJob ();
	}
	
  }
  catch (Words::UnsupportedFeature& e) {
	std::cerr << e.what () << std::endl;
	Words::Host::Terminate (Words::Host::ExitCode::UnsupportedFeature,std::cerr);
  }
  catch (Words::WordException& e) {
	std::cerr << e.what () << std::endl;
	return -1;
  }
  
}

