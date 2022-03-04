#include <sstream>
#include <iostream>
#include <set>

#include <numeric>

#include "words/algorithms.hpp"
#include "words/exceptions.hpp"
#include "words/words.hpp"
#include "words/linconstraint.hpp"
#include "passed.hpp"
#include "graph.hpp"
#include "solver.hpp"
#include "rules.hpp"
#include "heuristics.hpp"
#include "solvers/simplifiers.hpp"
#include "smt/smtsolvers.hpp"
#include "words/algorithms.hpp"
#include "host/trace.hpp"

#include <sstream>
#include <iostream>

#include <numeric>

namespace Words {
  namespace Solvers {
    namespace Levis {

      inline bool linearsSatisfiedByEmpty (const Words::Options& opt) {
	auto cBegin = opt.constraints.begin();
	auto cEnd   = opt.constraints.end();
	for (auto it = cBegin; it!=cEnd; ++it){
	  if ((*it) ->isLinear ()) {
	    auto lin  = (*it) ->getLinconstraint ();
	    if (0> lin->getRHS ())
	      return false;
	  }
	}
	return true;
      }

      Words::SMT::SolverResult solveDummy (const Words::Options& opt, Words::Substitution& s) {
	std::set<const Words::IEntry*> unrestricted;
	auto intsolver = Words::SMT::makeIntSolver ();
	for (auto& t : opt.constraints) {
	  if (t->isLinear()) {
	    auto lin = t->getLinconstraint ();
	    for (auto& vvar : *lin) {
	      if (!unrestricted.count (vvar.entry) ) { 
		intsolver->addVariable ((const Variable*)vvar.entry);
		unrestricted.insert (vvar.entry);
	      }
	    }
	    auto lconstraint = lin;
	    if (lconstraint->lhsEmpty ()) {
	      if (lconstraint->getRHS () < 0) {
		return Words::SMT::SolverResult::NSatis;
	      }
	    }
	    else 
	      intsolver->addConstraint (*lconstraint);
	  }
	}
	auto res = intsolver->solve ();
	if (res == Words::SMT::SolverResult::Satis) {
	  for (auto var : unrestricted) {
	    Words::Word w;
	    auto wb = const_cast<Words::Context&> (*opt.context).makeWordBuilder (w);
	    for (size_t i = 0; i < intsolver->evaluate (const_cast<Variable*>(static_cast<const Words::Variable*> (var))); ++i) {
	      *wb << '_';
	    }
	    wb->flush ();
	    s[const_cast<IEntry*> (var)] = w;
	  }
		  
	}
	return res;
      }
	  
      class Handler {
      public:
        Handler (PassedWaiting& w, Graph& g,Words::Substitution& s) : waiting(w),graph(g),subs(s) {
	  smtSolverCalls = 0;
        }
        // criteria for external solver calls
        //
	
        // trigger the solver with a timeout given via parameter every time the
        // waiting list exceeds a bound
        /*bool waitingListLimitExceeded(std::size_t wSize, std::size_t bound=35){
	  return wSize > bound;
	  }*/

        // sets the solver timeout based on items in the waiting list
        /*bool waitingListLimitExceededScaleTimeout(std::size_t wSize, std::size_t& timeout, std::size_t bound=35){
	  if(!waitingListLimitExceeded(wSize,bound))
	  return false;
	  timeout = (wSize-bound)*10;
	  return true;
	  }*/

        /*bool variableLimitReached(std::shared_ptr<Words::Options>& to, std::size_t bound=3){
	  return to->context.getVariableAlphabet().size() < bound;
	  }*/

        // some equations (maximumEquations amount) exceeded a length bound
        /*bool equationLengthExceeded(std::shared_ptr<Words::Options>& to, std::size_t maximumLength=50, std::size_t maximumEquations=0){
	//Words::Algorithms::ParikhImage p_lhs;
	auto eqBegin = to->equations.begin();
	auto eqEnd = to->equations.end();
	for(auto it = eqBegin; it != eqEnd; ++it){
	if (it->lhs.characters() + it->rhs.characters() > maximumLength){
	if (!maximumEquations)
	return true;
	maximumEquations--;
	}
	}
	return false;
	}*/

        size_t calculateTotalEquationSystemSize(const Words::Options& o){
	  size_t eqsSize = 0;
	  auto eqBegin = o.equations.begin();
	  auto eqEnd = o.equations.end();
	  for(auto it = eqBegin; it != eqEnd; ++it){
	    eqsSize = eqsSize + it->lhs.characters() + it->rhs.characters();
	  }
	  return eqsSize;
        }

        // Equation grows too quickly...
        bool equationLengthGrowthExceeded(const Words::Options& from, const std::shared_ptr<Words::Options>& to, double scale=1.25){
	  std::size_t fromSize = calculateTotalEquationSystemSize(from);
	  std::size_t toSize = calculateTotalEquationSystemSize(*to);
	  return fromSize*scale < toSize;
        }



        // Linear constraints handling
        bool modifyLinearConstraints(std::shared_ptr<Words::Options>& opt, const Words::Substitution& sub){
	  if (opt->constraints.size() == 0 || sub.size() == 0)
	    return true;
		  
		  
		  
	  std::vector<Constraints::Constraint_ptr> newConstraints;
		  
	  //std::cout << sub << std::endl;

	  auto cit = opt->constraints.begin();
	  auto cend = opt->constraints.end ();

	  for (; cit != cend; ++cit) {
	    if ((*cit)->isLinear ()) {
	      auto lin = (*cit)->getLinconstraint();
	      auto builder = Words::Constraints::makeLinConstraintBuilder (Words::Constraints::Cmp::LEq);
	      builder->addRHS (lin->getRHS ());
	      for (auto& vvar : *lin) {
		if (!sub.count(vvar.entry)) {
		  builder->addLHS (vvar.entry,vvar.number);
		}
		else {
		  for (const auto entry : sub.at(vvar.entry)) {
		    if (entry->isVariable ()) {
		      builder->addLHS (entry,vvar.number);
		    }
		    else {
		      builder->addRHS (-vvar.number);
		    }
		  }
		}
	      }
	      auto constr = builder->makeConstraint ();
	      auto lconstr = constr->getLinconstraint ();
	      if (lconstr->lhsEmpty () ) {
		if (lconstr->getRHS () < 0)
		  return false;
	      }
	      else 
		newConstraints.push_back (constr);
	    }
	    else {
	      newConstraints.push_back (*cit);
	    }
	  }
	  opt->constraints = newConstraints;
	  return true;
		  
        }
		
	//returns true if successor generation should stop
	//
	bool handle (const Words::Options& from, std::shared_ptr<Words::Options>& to, const Words::Substitution& sub) {
          //auto beforeSimp = to->copy ();
	  // Simplification
          if(!modifyLinearConstraints(to, sub))
	    return false;
		  
	/*
	          std::cout << "#######################################"<< std::endl;
		    std::cout << "Start modification on:" << from << std::endl;
		    std::cout << "=====================" <<  std::endl; 
	  */
	  // modifyLinearConstraints(to, sub);
          auto beforeSimp = to->copy ();
	    /*      std::cout << "First modification:" << *to << std::endl;
		    std::cout << "Substitution was: " << sub << std::endl;
		    std::cout << "=====================" <<  std::endl;
	  */		
	  Words::Substitution simplSub;
	  std::vector<Constraints::Constraint_ptr> ptr;
	  auto res = Words::Solvers::CoreSimplifier::solverReduce (*to,simplSub,ptr);
		  
	  //std::copy(ptr.begin(),ptr.end(),std::back_inserter (to->constraints));
	    /*std::cout << "Second modification:" << *to << std::endl;
	       std::cout << "Substitution was: " << simplSub << std::endl;
	       std::cout << "###########################################"<< std::endl;
	     */	
	  if(!modifyLinearConstraints(to, simplSub))
	    return false;

	  if (res==Simplified::ReducedNsatis){
	    	//std::cout << "c Simplifier reported UNSAT" << std::endl;
		 return false;
	  }
		  
          if (waiting.contains(to)){
	    	 //std::cout << "???" << std::endl;
		 return false;
	   }
	  
	  auto node = graph.getNode (from);			
	  auto simpnode = graph.makeNode (beforeSimp);
	  auto nnode = graph.makeNode (to);
		  
	  Edge* edge = graph.addEdge (node,simpnode,sub);

          if (simpnode != nnode)
            edge = graph.addEdge (simpnode,nnode,simplSub);
		  
	  SMTHeuristic& heur = getSMTHeuristic ();
	  Words::Substitution solution;
          
	  
	  if (res==Simplified::ReducedSatis ) {
            if (linearsSatisfiedByEmpty (*to)) {
	      result = Words::Solvers::Result::HasSolution;
	      subs = findRootSolution (nnode);
	      // rebuild subsitution here!>
	      waiting.clear();
	      return true;
	    }
	    else if (solveDummy (*to,solution) == Words::SMT::SolverResult::Satis ) {
	      auto dnode = graph.makeDummyNode ();
	      graph.addEdge (nnode,dnode,solution);
	      result = Words::Solvers::Result::HasSolution;
	      subs = findRootSolution (dnode);
	      waiting.clear();
	      return true;
	    }
			
			
	  }

		  
          else if  (heur.doRunSMTSolver (from,*to,waiting)) {
            return runSMTSolver (nnode,to,heur);
          }
	  else {
	    waiting.insert(to);
	  }



	  //TODO Check if the equation is
	  // Trivially satisfied,
	  // Trivially unsatisfied, or
	  // If we should call the SMT solver
	  // In either case of satisfied we need som code to reconstruct the substitution
	  // For now we just return false thus apply rules until we reach a fix-point
	  return false;
	}

        bool runSMTSolver (Node* n, const std::shared_ptr<Words::Options>& from, SMTHeuristic& heur) {
          smtSolverCalls = smtSolverCalls+1;
	  n->ranSMTSolver = true;
	  auto smtsolver = Words::SMT::makeSolver ();
	  heur.configureSolver (smtsolver);
	  Words::SMT::buildEquationSystem (*smtsolver,*from);
          switch (smtsolver->solve()) {
	  case Words::SMT::SolverResult::Satis: {
	    std::shared_ptr<Words::Options> tt = from->copy ();
	    tt->equations.clear();
			
	    Words::Substitution finalSolution;
	    Words::SMT::retriveSubstitution (*smtsolver,*tt,finalSolution);
	    auto nnode = graph.makeNode (tt);
	    graph.addEdge (n,nnode,finalSolution);
	    subs = findRootSolution (nnode);
	    waiting.clear();
	    result = Words::Solvers::Result::HasSolution;
	    return true;
	    break;
	  }
	  case Words::SMT::SolverResult::Unknown:
	    waiting.insert (from);
	    break;
	  case Words::SMT::SolverResult::NSatis:
	    break;
	  } 
	  return false;
	}
		
        auto getResult () const {
	  return result;
        }

        auto getSMTSolverCalls () const {
	  return smtSolverCalls;
        }

      private:
	PassedWaiting& waiting;
	Graph& graph;
	Words::Substitution& subs;
	Words::Solvers::Result result = Words::Solvers::Result::NoIdea;
        size_t smtSolverCalls;
      };
	  
      ::Words::Solvers::Result Solver::Solve (Words::Options& opt,::Words::Solvers::MessageRelay& relay)   {
	relay.pushMessage ("Levis Algorithm");
	relay.pushMessage ((Formatter ("Using Heuristic: %1%") % getSMTHeuristic().getDescription ()).str());
	relay.pushMessage ((Formatter ("Using SearchStrategy: %1%") % getQueue().getName ()).str());
	if (opt.hasIneqquality ())
	  return ::Words::Solvers::Result::NoIdea;
	if (opt.hasIneqquality ())
	  return ::Words::Solvers::Result::NoIdea;
	PassedWaiting waiting (getQueue());
	Graph graph;
        Handler handler (waiting,graph,sub);
        smtSolverCalls = 0;

	if (opt.equations.size() == 0) {
	  auto res = solveDummy (opt,sub); 
	  if ( res == Words::SMT::SolverResult::Satis ) {
	    return Words::Solvers::Result::HasSolution;
	  }
	  else if (res == Words::SMT::SolverResult::Unknown) {
	    return Words::Solvers::Result::NoIdea;
	  }
	  else {
	    return Words::Solvers::Result::DefinitelyNoSolution;
	  }
	  
	  
	}

#ifdef ENABLEGRAPH
	GuaranteeOutput go ("Levis",graph);
#endif
		
        // We need the substituion of the first simplifier run. This is just a quick hack...
        // Start THE SOLVER without simplify flag
        Words::Substitution simplSub;
	std::vector<Constraints::Constraint_ptr> cstr;
	auto first = opt.copy();
	auto insert = opt.copy ();
        auto res = Words::Solvers::CoreSimplifier::solverReduce (*insert,simplSub,cstr);
	//std::copy(cstr.begin(),cstr.end(),std::back_inserter (insert->constraints));

        if (!handler.modifyLinearConstraints(insert, simplSub)){
	  return Words::Solvers::Result::DefinitelyNoSolution;
        }

	auto inode = graph.makeNode (insert);

        if (res==Simplified::ReducedNsatis){
	  return Words::Solvers::Result::DefinitelyNoSolution;
        }
	else if (res==Simplified::ReducedSatis) {
	  if (linearsSatisfiedByEmpty (*insert)) {
	    auto fnode = graph.makeNode (first); 
	    graph.addEdge (fnode,inode,simplSub);
	    sub = findRootSolution (inode);
	    return Words::Solvers::Result::HasSolution;
          } else {
	    Words::Substitution solution;
	    auto fnode = graph.makeNode (first);
	    graph.addEdge (fnode,inode,simplSub);
	    auto res = solveDummy (*insert,solution); 
	    if ( res == Words::SMT::SolverResult::Satis) {
	      auto dnode = graph.makeDummyNode ();
	      graph.addEdge (inode,dnode,solution);
	      sub = findRootSolution (dnode);
	      return Words::Solvers::Result::HasSolution;
	    }
	    else if (res == Words::SMT::SolverResult::NSatis) {
	      return Words::Solvers::Result::DefinitelyNoSolution;
	    }			
	    else {
	      return Words::Solvers::Result::NoIdea;
	    }
	  }
	}
	//auto insert = opt.copy ();
	else {
	  waiting.insert (insert);
	}
	while (waiting.size()) {
          auto cur = waiting.pullElement ();
	
	//std::cout << "---------" << std::endl;
	//std::cout << "Consider the equation: " << waiting.size() << " " << *cur << std::endl;

	  RuleSequencer<Handler,PrefixReasoningLeftHandSide,PrefixReasoningRightHandSide,PrefixReasoningEqual,PrefixEmptyWordLeftHandSide,PrefixEmptyWordRightHandSide,PrefixLetterLeftHandSide,PrefixLetterRightHandSide,SuffixReasoningLeftHandSide,SuffixReasoningRightHandSide,SuffixReasoningEqual,SuffixEmptyWordLeftHandSide,SuffixEmptyWordRightHandSide,SuffixLetterLeftHandSide,SuffixLetterRightHandSide>::runRules (handler,*cur);

         // RuleSequencer<Handler,GuessConstIsOneVariable,PrefixReasoningLeftHandSide,PrefixReasoningRightHandSide,PrefixReasoningEqual,PrefixEmptyWordLeftHandSide,PrefixEmptyWordRightHandSide,PrefixLetterLeftHandSide,PrefixLetterRightHandSide,SuffixReasoningLeftHandSide,SuffixReasoningRightHandSide,SuffixReasoningEqual,SuffixEmptyWordLeftHandSide,SuffixEmptyWordRightHandSide,SuffixLetterLeftHandSide,SuffixLetterRightHandSide>::runRules (handler,*cur);
          relay.progressMessage ((Words::Solvers::Formatter ("Passed: %1%, Waiting: %2%") % waiting.passedsize() % waiting.size()).str());
	}

        smtSolverCalls = handler.getSMTSolverCalls();

        if (handler.getResult() == Words::Solvers::Result::NoIdea ){
	  return Words::Solvers::Result::DefinitelyNoSolution;
        }


        return handler.getResult ();
      }





    }
	
  }
}
