#include <sstream>
#include <iostream>

#include "words/exceptions.hpp"
#include "words/words.hpp"
#include "words/linconstraint.hpp"
#include "solvers/exceptions.hpp"
#include "core/Solver.h"
#include "solver.hpp"

void setupSolverMain (std::vector<std::string>&, std::vector<std::string>&);
void clearLinears();
void addLinearConstraint (std::vector<std::pair<char, int>> lhs, int rhs);

template<bool>
::Words::Solvers::Result runSolver (const bool squareAuto, size_t bound, const Words::Context&,Words::Substitution&,Words::Solvers::Timing::Keeper&,std::ostream*);
  
namespace Words {
  namespace Solvers {
	namespace SatEncoding {
	  template<bool encoding>
	  ::Words::Solvers::Result Solver<encoding>::Solve (Words::Options& opt,::Words::Solvers::MessageRelay& relay)   {
		relay.pushMessage ("SatSolver Ready");
		if (!opt.context.conformsToConventions ())  {
		  relay.pushMessage ("Context does not conform to Upper/Lower-case convention");
		  return ::Words::Solvers::Result::NoIdea;
		}

		Words::IEntry* entry = nullptr;
	
		std::stringstream str;
		std::vector<std::string> lhs;
		std::vector<std::string> rhs;
		
		for (auto& eq : opt.equations) {
		  str.str("");
		  for (auto e : eq.lhs) {
			if (!entry && e->isTerminal ())
			  entry = e;
			str << e->getRepr ();
		  }
		  lhs.push_back (str.str());
		  str.str("");
		  for (auto e : eq.rhs) {
			if (!entry && e->isTerminal ())
			  entry = e;
			str << e->getRepr ();
		  }
		  rhs.push_back (str.str());
		}

		clearLinears();
		
		for (auto& constraint : opt.constraints) {
		  if (!handleConstraint (*constraint,relay,entry))
			return ::Words::Solvers::Result::NoIdea;
		}
		
		setupSolverMain (lhs,rhs);
		Words::Solvers::Timing::Timer overalltimer (timekeep,"Overall Solving Time");

		/*int b = static_cast<int> (bound);
		int currentBound = 0;
		int i = 0;
		Words::Solvers::Result ret;

		while(i < b){
			i++;
			currentBound = std::pow(i,2);
			try {
			  ret =  runSolver<encoding> (false,static_cast<size_t> (currentBound),opt.context,sub,timekeep,(diagnostic ? &diagStr : nullptr));
			  if(ret == Words::Solvers::Result::HasSolution){
				  std::cout << "LOL" << std::endl;
				return ret;
			  }
			}catch(Glucose::OutOfMemoryException& e) {
			  throw Words::Solvers::OutOfMemoryException ();
			}
		}
		return ret;
		*/

		try {
		  return runSolver<encoding> (false,bound,opt.context,sub,timekeep,(diagnostic ? &diagStr : nullptr));
		}catch(Glucose::OutOfMemoryException& e) {
		  throw Words::Solvers::OutOfMemoryException ();
		}


	  }
	  
	  //Should only be called if Result returned HasSolution
	  template<bool encoding>
	  void Solver<encoding>::getResults (Words::Solvers::ResultGatherer& r)  {
		r.setSubstitution (sub);
		r.timingInfo (timekeep);
		if(diagnostic)
		  r.diagnosticString (diagStr.str());
	  }
	  
	  template<bool encoding>
	  bool Solver<encoding>::handleConstraint (Words::Constraints::Constraint& c, ::Words::Solvers::MessageRelay& relay,Words::IEntry* e ) {
		//std::cerr << "Handle " << c << std::endl; 
		Words::Constraints::LinearConstraint* lc = c.getLinconstraint ();
		if (lc) {
		  if (!e) {
			relay.pushMessage ("Missing Dummy Variable");
			return false;
		  }
		  std::vector<std::pair<char, int>> lhs;
		  int rhs = lc->getRHS ();
		  for (auto& vm : *lc) {
			  char variableName = vm.entry->getRepr ();
			  int coefficient = vm.number;
			  //std::cout << coefficient << " " << variableName << std::endl;

			  lhs.push_back(std::make_pair(variableName,coefficient));
		  }
		  addLinearConstraint(lhs,rhs);
		  return true;
		}
		else {
		  relay.pushMessage ("Only Lineary Constraints are supported");
		  return false;
		}


	  
	  }
	}
	
  }
}
