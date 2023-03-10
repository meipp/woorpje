#include <sstream>
#include <iostream>

#include <numeric>

#include "words/exceptions.hpp"
#include "words/words.hpp"
#include "words/linconstraint.hpp"
#include "solvers/simplifiers.hpp"
#include "solvers/exceptions.hpp"
#include "core/Solver.h"
#include "solver.hpp"
#include "regular/regencoding.h"
//#include "regular/commons.h"


namespace commons {
    void profileToCsv(const std::vector<RegularEncoding::EncodingProfiler> &profiles, std::string prefix = "");
}



Words::Solvers::Result setupSolverMain(Words::Options &opt); //std::vector<std::string>&, std::vector<std::string>&);
void clearLinears();

void addLinearConstraint(std::vector<std::pair<Words::Variable *, int>> lhs, int rhs);

template<bool>
::Words::Solvers::Result runSolver(const bool squareAuto, size_t bound, const Words::Context &, Words::Substitution &,
                                   Words::Solvers::Timing::Keeper &, std::ostream *,
                                   RegularEncoding::EncodingProfiler *);



namespace Words {
    namespace Solvers {
        namespace SatEncoding {
            template<bool encoding>
            ::Words::Solvers::Result
            Solver<encoding>::Solve(Words::Options &opt, ::Words::Solvers::MessageRelay &relay) {
                relay.pushMessage("SatSolver Ready");
                if (opt.hasIneqquality())
                    return ::Words::Solvers::Result::NoIdea;
                /*if (!opt.context->conformsToConventions ())  {
                  relay.pushMessage ("Context does not conform to Upper/Lower-case convention");
                  return ::Words::Solvers::Result::NoIdea;
                }*/

                Words::IEntry *entry = opt.context->getEpsilon();

                // Unused?
                std::stringstream str;
                std::vector<std::string> lhs;
                std::vector<std::string> rhs;

                // hack to add lost variables due to simplificaiton
                auto ctx = opt.context;
                for (auto &x: ctx->getVariableAlphabet()) {
                    Words::Word side({x});
                    Words::Equation eq(side, side);
                    opt.equations.push_back(eq);
                }


                // we need at least one terminal symbol
                //
                std::cout << ctx->getTerminalAlphabet().size() << std::endl;
                if (ctx->getTerminalAlphabet().size() == 1 && opt.constraints.size() > 0) {
                    std::cout << "xxx" << std::endl;
                    ctx->addTerminal('a');
                }


                for (auto &eq: opt.equations) {
		    str.str("");
                    for (auto e: eq.lhs) {
                        str << e->getName();
                    }
                    lhs.push_back(str.str());
                    str.str("");
                    for (auto e: eq.rhs) {
                        str << e->getName();
                    }
                    rhs.push_back(str.str());
                }

                Words::Solvers::Result retPreprocessing = setupSolverMain(opt); //(lhs,rhs);
                // preprocessing match
                if (retPreprocessing != Words::Solvers::Result::NoIdea) {
                    return retPreprocessing;
                }

                clearLinears();

                for (auto &constraint: opt.constraints) {
                    if (!handleConstraint(*constraint, relay, entry))
                        return ::Words::Solvers::Result::NoIdea;
                }


                Words::Solvers::Timing::Timer overalltimer(timekeep, "Overall Solving Time");


                const int actualb = (bound ?
                                     static_cast<int> (bound) :

                                     static_cast<int> (std::accumulate(
                                             opt.equations.begin(),
                                             opt.equations.end(),
                                             0,
                                             [](size_t s, const Words::Equation &eq) {
                                                 return eq.lhs.characters() + eq.rhs.characters() + s;
                                             })
                                     )
                );

                int actualbre = 0;
                int lowerBound = -1; 
                for (const auto &recon: opt.recons) {
                    recon->expr->flatten();
                    int maxm = recon->expr->longestLiteral();
                    int minm = recon->expr->shortestLiteral();
                    if (maxm > actualbre) {
                        actualbre = maxm;
                    }
                    if (lowerBound < 0 || minm < lowerBound) {
                        lowerBound = minm;
                    }
                }

                actualbre = (int) std::ceil(std::sqrt(actualbre)) + 1;


                if (lowerBound < 0){
                    lowerBound = 1;
                }
                int i = (int) std::ceil(std::sqrt(lowerBound));


                Words::Solvers::Result ret = Words::Solvers::Result::NoSolution;
                std::vector<RegularEncoding::EncodingProfiler> profilers;
                while (i < actualb || i < actualbre) {
                    i++;
                    int currentBound = std::pow(i, 2);
                    try {
                        RegularEncoding::EncodingProfiler profiler{};
                        ret = runSolver<encoding>(false, static_cast<size_t> (currentBound), *opt.context, sub,
                                                  timekeep, (diagnostic ? &diagStr : nullptr), &profiler);
                        profilers.push_back(profiler);
                        if (ret == Words::Solvers::Result::HasSolution) {
                            commons::profileToCsv(profilers);
                            return ret;
                        }  else if (ret == Words::Solvers::Result::DefinitelyNoSolution) {
                            return ret;
                        }
                    } catch (Glucose::OutOfMemoryException &e) {
                        throw Words::Solvers::OutOfMemoryException();
                    }
                }
                commons::profileToCsv(profilers);
                return ret;

            }

            //Should only be called if Result returned HasSolution
            template<bool encoding>
            void Solver<encoding>::getResults(Words::Solvers::ResultGatherer &r) {
                r.setSubstitution(sub);
                r.timingInfo(timekeep);
                if (diagnostic)
                    r.diagnosticString(diagStr.str());
            }

            template<bool encoding>
            bool
            Solver<encoding>::handleConstraint(Words::Constraints::Constraint &c, ::Words::Solvers::MessageRelay &relay,
                                               Words::IEntry *e) {
                //std::cerr << "Handle " << c << std::endl;
                const Words::Constraints::LinearConstraint *lc = c.getLinconstraint();
                if (lc) {
                    if (!e) {
                        relay.pushMessage("Missing Dummy Variable");
                        return false;
                    }
                    std::vector<std::pair<Words::Variable *, int>> lhs;
                    int rhs = lc->getRHS();
                    for (auto &vm: *lc) {
                        int coefficient = vm.number;
                        lhs.push_back(std::make_pair(vm.entry->getVariable(), coefficient));
                    }
                    addLinearConstraint(lhs, rhs);

                    return true;
                } else {
                    relay.pushMessage("Only Lineary Constraints are supported");
                    return false;
                }
            }
        }

    }
}
