

#include "words/words.hpp"
#include "solvers/solvers.hpp"
#include "core.hpp"

namespace Words{
  namespace Reach {
	class Solver : public ::Words::Solvers::Solver {
	public:
	  Solver (size_t bound) : bound(bound) {} 
	  Words::Solvers::Result Solve (Words::Options& c,Words::Solvers::MessageRelay&) override {
		SearchState<Words::IEntry> search;
		return Words::Solvers::Result::NoIdea;
	  }
	  
	  //Should only be called if Result returned HasSolution
	  void getResults (Words::Solvers::ResultGatherer& r) override {
		
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
