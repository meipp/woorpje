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
		virtual std::string getDescription () const  {return "Unknown";}
	  };

	  using SMTHeuristic_ptr = std::unique_ptr<SMTHeuristic>;
	  SMTHeuristic& getSMTHeuristic ();
	  class WaitingListLimitReached : public SMTHeuristic {
	  public:
		WaitingListLimitReached (size_t b) : bound (b) {
		}
		bool doRunSMTSolver (const Words::Options& from,const Words::Options& opt, const PassedWaiting& pw) const override {
		  return pw.size() > bound;
		}
		
		virtual void configureSolver (Words::SMT::Solver_ptr&) override {}
		virtual std::string getDescription () const override {return (Formatter ("WaitingListLimit - %1%") % bound).str();};
	  private:
		size_t bound;
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
		
		virtual void configureSolver (Words::SMT::Solver_ptr& solver) override {
		  solver->setTimeout (timeout);
		}
		
	  private:
		size_t bound = 0;
		mutable size_t timeout  = 0;
		
	  };

	  class EquationLengthExceeded : public SMTHeuristic {
	  public:
        EquationLengthExceeded (size_t eqLength) : eqLengthBound (eqLength) {}
		bool doRunSMTSolver (const Words::Options& from,const Words::Options& opt, const PassedWaiting& pw) const override {
		  auto eqBegin = opt.equations.begin();
		  auto eqEnd = opt.equations.end();
          int maximumEquations = opt.equations.size() / 2;
		  for(auto it = eqBegin; it != eqEnd; ++it){
			if (it->lhs.characters() + it->rhs.characters() > eqLengthBound){
			  if (!maximumEquations)
				return true;
			  maximumEquations--;
			}
		  }
		  return false;
		}


		virtual void configureSolver (Words::SMT::Solver_ptr&) override {
		}
        virtual std::string getDescription () const override {return (Formatter ("Fixed equation length exceeded - %1%") % eqLengthBound).str();};
		
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
		
		virtual void configureSolver (Words::SMT::Solver_ptr&) override {}
        virtual std::string getDescription () const override {return (Formatter ("Equation growth - %1%") % scale).str();};

	  private:
		double scale = 1.25;
	  };


      class VariableTerminalRatio : public SMTHeuristic {
      public:
        VariableTerminalRatio (double scale) : scale(scale) {}
        bool doRunSMTSolver (const Words::Options& from,const Words::Options& opt, const PassedWaiting& pw) const override {
          std::size_t fTerminals, tTerminals, fVariables, tVariables = 0;
          calculateTotalCount(from,fTerminals,fVariables);
          calculateTotalCount(opt,tTerminals,tVariables);

          if(fVariables == 0 || tVariables == 0)
              return false;

          return (fTerminals/fVariables)*scale < (tTerminals/tVariables);
        }

		virtual std::string getDescription () const override {return (Formatter ("VariableTerminalRatio - %1%") % scale).str();};
		
        void calculateTotalCount(const Words::Options& o, size_t & terminals, size_t & variables) const {
          auto eqBegin = o.equations.begin();
          auto eqEnd = o.equations.end();
          for(auto it = eqBegin; it != eqEnd; ++it){
              it->lhs.sepearteCharacterCount(terminals,variables);
          }
        }

        virtual void configureSolver (Words::SMT::Solver_ptr&) override {}

      private:
        double scale = 1.25;
      };


      class NoSMT : public SMTHeuristic {
      public:
        NoSMT () {}
        bool doRunSMTSolver (const Words::Options& from,const Words::Options& opt, const PassedWaiting& pw) const override {
          return false;
        }

        virtual std::string getDescription () const override {return "No SMT Solver";};


        virtual void configureSolver (Words::SMT::Solver_ptr&) override {}
      };
    }
  }
}

#endif
