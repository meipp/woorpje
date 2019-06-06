#ifndef _HEURISTICS__
#define _HEURISTICS__

#include <memory>

#include "words/words.hpp"
#include "smt/smtsolvers.hpp"
#include "passed.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {
	  class SMTHeuristic {
	  public:
		virtual bool doRunSMTSolver (const Words::Options& from,const Words::Options&, const PassedWaiting& ) const { return true;}
		virtual void configureSolver (Words::SMT::Solver_ptr&) {}
	  };

using SMTHeuristic_ptr = std::unique_ptr<SMTHeuristic>;
	  
	  class WaitingListLimitReached : public SMTHeuristic {
	  public:
		WaitingListLimitReached (size_t bound) : bound (bound) {}
		bool doRunSMTSolver (const Words::Options& from,const Words::Options& opt, const PassedWaiting& pw) const override {
		  return pw.size() > bound;
		}

		virtual void configureSolver (Words::SMT::Solver_ptr&) {}
		
	  private:
		size_t bound = 0;
	  };

	  class WaitingListLimitReachedScaledTimeout : public SMTHeuristic {
	  public:
		WaitingListLimitReachedScaledTimeout (size_t bound) : bound(bound) {}
		bool doRunSMTSolver (const Words::Options& from,const Words::Options& opt, const PassedWaiting& pw) const override {
		  if (pw.size() >= bound) {
			timeout = (pw.size()-bound)*10;
			return true;
		  }
		  
		  return false;
			  
		}
		
		virtual void configureSolver (Words::SMT::Solver_ptr& solver) {
		  solver->setTimeout (timeout);
		}
		
	  private:
		size_t bound = 0;
		mutable size_t timeout  = 0;
		
	  };

	  class EquationLengthExceeded : public SMTHeuristic {
	  public:
		EquationLengthExceeded (size_t eqLength, size_t maxEquations ) : eqLengthBound (eqLength),
																		 maxEquations (maxEquations) {}
		bool doRunSMTSolver (const Words::Options& from,const Words::Options& opt, const PassedWaiting& pw) const override {
		  auto maximumEquations = maxEquations;
		  auto eqBegin = opt.equations.begin();
		  auto eqEnd = opt.equations.end();
		  for(auto it = eqBegin; it != eqEnd; ++it){
			if (it->lhs.characters() + it->rhs.characters() > eqLengthBound){
			  if (!maximumEquations)
				return true;
			  maximumEquations--;
			}
		  }
		  return false;
		}

		virtual void configureSolver (Words::SMT::Solver_ptr&) {
		}
		
	  private:
		size_t eqLengthBound = 0;
		size_t maxEquations = 0;
	  };

	  class CalculateTotalEquationSystemSize : public SMTHeuristic {
	  public:
		CalculateTotalEquationSystemSize (double scale) : scale(scale) {}
		bool doRunSMTSolver (const Words::Options& from,const Words::Options& opt, const PassedWaiting& pw) const override {
		  std::size_t fromSize = calculateTotalEquationSystemSize(from);
		  std::size_t toSize = calculateTotalEquationSystemSize(opt);
            return fromSize*scale < toSize;
		}

		
		size_t calculateTotalEquationSystemSize(const Words::Options& o) const {
		  size_t eqsSize = 0;
		  auto eqBegin = o.equations.begin();
		  auto eqEnd = o.equations.end();
		  for(auto it = eqBegin; it != eqEnd; ++it){
			eqsSize = eqsSize + it->lhs.characters() + it->rhs.characters();
		  }
		  return eqsSize;
        }
		
		virtual void configureSolver (Words::SMT::Solver_ptr&) {}
		
	  private:
		double scale = 1.25;
	  };
	  
	}
  }
}

#endif
