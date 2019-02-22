
#include <iostream>

#include "words/words.hpp"
#include "solvers/solvers.hpp"
#include "core.hpp"

namespace Words{
  namespace Reach {
	class Solver : public ::Words::Solvers::Solver {
	public:
	  Solver (size_t bound) : bound(bound) {} 
	  Words::Solvers::Result Solve (Words::Options& c,Words::Solvers::MessageRelay&) override {
		PassedWaiting passed;
		SuccGenerator gen (c.context,c.equations[0].lhs,c.equations[0].rhs,bound,passed);;
		gen.makeInitial ();
		SearchStatePrinter<Words::IEntry> printer (std::cerr);
		
		while (passed.hasWaiting ()) {
		  auto st = passed.pull ();
		  std::cerr << "=====" << std::endl;
		  printer.output (c.context,*st,bound);
		  if (!gen.finalState (*st)) {
			
			gen.step(*st);
		  }
		  else {
			for (size_t i = 0; i < c.context.nbVars(); i++) {
			  auto var = c.context.getVariable (i);
			  std::vector<Words::IEntry*> entries;
			  for (size_t j = 0; j < bound; j++) {
				auto entry = (*st->substitutions[var->getIndex()])[j];
				if (entry != nullptr && entry != c.context.getEpsilon ()) {
				  entries.push_back (const_cast<IEntry*> (entry));
				}
			  }
			  sub.emplace (var, std::move(entries));
			}

			
			return Words::Solvers::Result::HasSolution;
		  }
		}
		
		return Words::Solvers::Result::NoIdea;
	  }
	  
	  //Should only be called if Result returned HasSolution
	  void getResults (Words::Solvers::ResultGatherer& r) override {
		r.setSubstitution (sub);
		r.timingInfo (timekeep);
		
	  }
	  
	  virtual void enableDiagnosticOutput () {
		diagnostic= true;
	  }
	private:
	  bool handleConstraint (Words::Constraints::Constraint&, ::Words::Solvers::MessageRelay&,Words::IEntry* = nullptr);
	  Words::Substitution sub;
	  bool diagnostic = false;
	  Words::Solvers::Timing::Keeper timekeep;
	  size_t bound;
	  
	};

	
  }
  
}


namespace Words {
  namespace Solvers {
	template<>
	Solver_ptr makeSolver<Types::Reachability,size_t> (size_t bound ) {return std::make_unique<Words::Reach::Solver> (bound);}
  }
}
