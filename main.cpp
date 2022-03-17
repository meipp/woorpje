#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/program_options.hpp>

#include "words/words.hpp"
#include "words/exceptions.hpp"
#include "words/regconstraints.hpp"
#include "host/resources.hpp"
#include "host/exitcodes.hpp"
#include "parser/parsing.hpp"
#include "solvers/solvers.hpp"
#include "smt/smtsolvers.hpp"


#include "solvers/simplifiers.hpp"
#include "solvers/exceptions.hpp"


#include "config.h"

namespace po = boost::program_options;

struct LevisHeuristics {
    size_t which = 0;
    size_t searchorder = 0;
    double varTerminalRatio = 1.1;
    size_t wlistLimit = 2;
    double growthFactor = 1.1;
    size_t eqLength = 100;
};


class CoutResultGatherer : public Words::Solvers::DummyResultGatherer {
public:
    CoutResultGatherer(const Words::Options &opt, std::string &out, std::string &smtmodelfile) : opt(opt),
                                                                                                 outputfile(out),
                                                                                                 smtmodelfile(
                                                                                                         smtmodelfile) {}

    void setSubstitution(Words::Substitution &w) override {
        std::cout << "Substitution: " << std::endl;
        for (size_t var = 0; var < opt.context->nbVars(); var++) {
            auto variable = opt.context->getVariable(var);
            if (variable->isTemporary())
                continue;

            std::cout << *variable << ": ";
            for (auto c: w[variable]) {
                c->output(std::cout);
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;

        std::cout << "Equations after substition" << std::endl;
        for (auto &eq: opt.equations) {
            printWordWithSubstitution(eq.lhs, w);
            std::cout << eq.type;
            printWordWithSubstitution(eq.rhs, w);
            std::cout << std::endl;
        }
        std::cout << std::endl;

        if (smtmodelfile != "") {
            std::fstream str;
            str.open(smtmodelfile, std::ios::out);
            str << "(model\n";

            for (size_t var = 0; var < opt.context->nbVars(); var++) {
                auto variable = opt.context->getVariable(var);
                str << "(define-fun " << *variable << "() String \"";
                for (auto c: w[variable]) {
                    c->output(str);
                }
                str << "\")\n";
            }

            str << ")";

            str.close();
        }

        std::cout << "Output to: " << outputfile << std::endl;
        if (outputfile != "") {
            std::fstream str;
            str.open(outputfile, std::ios::out);
            Words::outputSMT(str, opt);
            str.close();
        }

    }

    void diagnosticString(const std::string &s) override {
        std::cout << "Diagnostic from Solvers" << std::endl;
        std::cout << s << std::endl;
    }

    void timingInfo(const Words::Solvers::Timing::Keeper &keep) override {
        std::cout << "Timing info" << std::endl;
        for (const auto &entry: keep) {
            std::cout << entry.name << " : " << entry.time << " milliseconds" << std::endl;
        }
        std::cout << std::endl;
    }

private:
    void printWordWithSubstitution(const Words::Word &word, Words::Substitution &w) {
        for (auto c: word) {
            if (c->isTerminal())
                c->output(std::cout);
            else {
                auto sub = w[c];
                for (auto s: sub) {
                    s->output(std::cout);
                }
            }
        }
    }

    const Words::Options &opt;
    std::string &outputfile;
    std::string &smtmodelfile;
};

void printContactDetails(std::ostream &os) {
    os << Version::ORGANISATION << std::endl;
    os << Version::MAINDEVELOPERNAME << " - " << Version::MAINDEVELOPEREMAIL << "" << std::endl;
}


void printBanner(std::ostream &os) {
    os << Version::TOOLNAME << " " << Version::VERSION_MAJOR << "." << Version::VERSION_MINOR << " (" << __DATE__ << ")"
       << std::endl;
    os << "Revision: " << Version::GIT_HASH << std::endl;
    os << Version::DEPSACK << std::endl;
    os << "SMT: ";
    try {
        os << Words::SMT::makeSolver()->getVersionString() << std::endl;
    } catch(Words::SMT::SMTSolverUnavailable &) {
        os << "not set\n";
    }

    printContactDetails(os);
    os << std::endl;
}

void setupLevis(LevisHeuristics &l) {
    using namespace Words::Solvers::Levis;
    switch (l.which) {
        case 0:
            selectVariableTerminalRatio(l.varTerminalRatio);
            break;
        case 1:
            selectWaitingListReached(l.wlistLimit);
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

    switch (l.searchorder) {

        case 1:
            setSearchOrder<SearchOrder::DepthFirst>();
            break;
        case 0:
        default:
            setSearchOrder<SearchOrder::BreadthFirst>();
            break;

    }


}

void printHelp(std::ostream &os, po::options_description &desc) {
    printBanner(os);
    os << desc;
}

void setSMTSolver(size_t i) {
    switch (i) {
        case 0:
            Words::SMT::setSMTSolver(Words::SMT::SMTSolver::Z3);
            break;
        case 2:
            Words::SMT::setSMTSolver(Words::SMT::SMTSolver::Z3Str3);
            break;
        case 1:
        default:
            Words::SMT::setSMTSolver(Words::SMT::SMTSolver::CVC4);
            break;
    }
}

Words::Solvers::Solver_ptr buildSolver(size_t i) {
    switch (i) {
        case 0:
            return nullptr;
            break;
        case 1:
            return Words::Solvers::makeSolver<Words::Solvers::Types::SatEncoding>(static_cast<size_t> (0));
        case 2:
            return Words::Solvers::makeSolver<Words::Solvers::Types::SatEncodingOld>(static_cast<size_t> (0));
        case 3:
            return Words::Solvers::makeSolver<Words::Solvers::Types::PureSMT>();
        case 4:
            return Words::Solvers::makeSolver<Words::Solvers::Types::Levis>();
    }
    return nullptr;
}


int main(int argc, char **argv) {
    bool diagnostic = false;
    bool suppressbanner = false;
    bool help = false;
    bool simplifier = false;
    size_t cpulim = 0;
    size_t vmlim = 0;
    size_t solverr = 0;
    std::string conffile;
    std::string outputfile = "";
    std::string smtmodelfile = "";

    po::options_description desc("General Options");
    LevisHeuristics lheu;
    desc.add_options()
            ("help,h", po::bool_switch(&help), "Help message.")
            ("nobanner,n", po::bool_switch(&suppressbanner), "Suppress the banner.")
            ("diagnostics,d", po::bool_switch(&diagnostic), "Enable Diagnostic Data.")
            ("configuration,c", po::value<std::string>(&conffile), "Configuration file")
            ("cpulim,C", po::value<size_t>(&cpulim), "CPU Limit in seconds")
            ("simplify", po::bool_switch(&simplifier), "Enable simplifications")
            ("vmlim,V", po::value<size_t>(&vmlim), "VM Limit in MBytes")

            ("outputfile", po::value<std::string>(&outputfile), "Output Satisfiable Equation to file")
            ("smtmodel", po::value<std::string>(&smtmodelfile), "Output model in SMTLib-format")

            ("solver", po::value<size_t>(&solverr), "Solver Strategy\n"
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
            ("smtsolver,S", po::value<size_t>(&smtsolver), "SMT Solver\n"
                                                           "\t 0 Z3\n"
                                                           "\t 1 CVC4\n"
                                                           "\t 2 Z3Str3\n"
            )
            ("smttimeout", po::value<size_t>(&smttimeout), "Set timeout for SMTSolver (ms)");

    po::options_description levdesc("LevisSMT Options");
    levdesc.add_options()
            ("levisheuristics", po::value<size_t>(&lheu.which), "Levi Heuristics\n"
                                                                "\t 0 VariableTerminalRatio\n"
                                                                "\t 1 WaitingListLimitReached\n"
                                                                "\t 2 EquationGrowth\n"
                                                                "\t 3 EquationLengthExceeded\n"
                                                                "\t 4 None\n"
            )
            ("VarTerminalRation", po::value<double>(&lheu.varTerminalRatio), "Variable Terminal Ratio")
            ("WaitingLimit", po::value<size_t>(&lheu.wlistLimit), "WaitingListLimit")
            ("growth", po::value<double>(&lheu.growthFactor), "Equation Growth Ratio")
            ("eqLength", po::value<size_t>(&lheu.eqLength), "Equation Length")
            ("SearchOrder", po::value<size_t>(&lheu.searchorder), "Search Order\n"
                                                                  "\t 0 BFS\n"
                                                                  "\t 1 DFS");


    desc.add(smdesc);
    desc.add(levdesc);
    po::positional_options_description positionalOptions;
    positionalOptions.add("configuration", 1);
    po::variables_map vm;


    po::store(po::command_line_parser(argc, argv).options(desc)
                      .positional(positionalOptions).run(), vm);
    po::notify(vm);


    if (help) {

        printHelp(std::cout, desc);
        return -1;
    }

    setupLevis(lheu);
    setSMTSolver(smtsolver);

    Words::SMT::setDefaultTimeout(smttimeout);
    if (!suppressbanner)
        printBanner(std::cout);
    if (conffile == "") {
        std::cerr << "Configuration file not specified" << std::endl;
        return -1;
    }

    if (cpulim && !Words::Host::setCPULimit(cpulim, std::cerr))
        Words::Host::Terminate(Words::Host::ExitCode::ConfigurationError, std::cout);

    if (vmlim && !Words::Host::setVMLimit(vmlim, std::cerr))
        Words::Host::Terminate(Words::Host::ExitCode::ConfigurationError, std::cout);




    // bool parsesucc = false; BD: not used
    try {

        std::fstream inp;
        inp.open(conffile);
        auto parser = Words::makeParser(Words::ParserType::Standard, inp);
        std::cout << "[*] Parsing input file \n";
        auto jg = parser->Parse(std::cout);
        std::cout << "[*] Parsing done \n";
        auto job = jg->newJob();

        for (std::shared_ptr <Words::RegularConstraints::RegConstraint> rc: job->options.recons) {
            rc->toString(std::cout);
            std::cout << "\n";
        }


        size_t noSolutionCount = 0;
        size_t DefinitelyNoSolutionCount = 0;
        size_t noIdeaCount = 0;
        size_t totalcount = 0;
        while (job) {
            totalcount++;
            if (!job->options.hasIneqquality()) {


                Words::Solvers::Solver_ptr solver = buildSolver(solverr);
                auto s = std::move(job->solver);

                if (!solver) {
                    std::cout << "Using Solver from input file." << std::endl;
                    std::swap(solver, s);
                } else {
                    std::cout << "Using command line forced  solver." << std::endl;
                }
                Words::Options foroutput = job->options;
                inp.close();

                CoutResultGatherer gatherer(foroutput, outputfile, smtmodelfile);
                if (solver) {
                    std::cout << "Solving Equation System" << std::endl << job->options << std::endl;;

                    if (simplifier) {

                        std::cout << "Running Simplifiers" << std::endl;
                        Words::Substitution sub;
                        std::vector <Words::Constraints::Constraint_ptr> cstr;
                        auto res = Words::Solvers::CoreSimplifier::solverReduce(job->options, sub, cstr);
                        std::cout << "Equation System after simplification" << std::endl << job->options << std::endl;;

                        switch (res) {
                            case ::Words::Solvers::Simplified::JustReduced:
                                break;
                            case ::Words::Solvers::Simplified::ReducedNsatis:
                                //Words::Host::Terminate (Words::Host::ExitCode::DefinitelyNoSolution,std::cout);
                                DefinitelyNoSolutionCount++;
                                break;
                            case ::Words::Solvers::Simplified::ReducedSatis:
                                gatherer.setSubstitution(sub);
                                Words::Host::Terminate(Words::Host::ExitCode::GotSolution, std::cout);
                        }
                        if (DefinitelyNoSolutionCount == totalcount) {
                            Words::Host::Terminate(Words::Host::ExitCode::DefinitelyNoSolution, std::cout);
                        }
                    }

                    if (diagnostic)
                        solver->enableDiagnosticOutput();

                    try {
                        Words::Solvers::StreamRelay relay(std::cout);
                        Words::Solvers::Result ret;


                        ret = solver->Solve(job->options, relay);

                        solver->getMoreInformation(std::cout);

                        std::cout << "\n" << std::endl;

                        switch (ret) {

                            case Words::Solvers::Result::HasSolution: {
                                solver->getResults(gatherer);
                                Words::Host::Terminate(Words::Host::ExitCode::GotSolution, std::cout);
                            }
                            case Words::Solvers::Result::NoSolution: {
                                //Words::Host::Terminate (Words::Host::ExitCode::NoSolution,std::cout);
                                noSolutionCount++;
                                break;
                            }
                            case Words::Solvers::Result::DefinitelyNoSolution: {
                                //Words::Host::Terminate (Words::Host::ExitCode::DefinitelyNoSolution,std::cout);
                                DefinitelyNoSolutionCount++;
                                break;
                            }

                            default:
                                //Words::Host::Terminate (Words::Host::ExitCode::NoIdea,std::cout);
                                noIdeaCount++;
                        }

                    } catch (Words::Solvers::OutOfMemoryException &) {
                        Words::Host::Terminate(Words::Host::ExitCode::OutOfMemory, std::cout);
                    }
                    catch (Words::UnsupportedFeature &o) {
                        std::cout << o.what() << std::endl;
                        Words::Host::Terminate(Words::Host::ExitCode::UnsupportedFeature, std::cout);
                    }
                    catch (Words::WordException &o) {
                        std::cout << o.what() << std::endl;
                        Words::Host::Terminate(Words::Host::ExitCode::ConfigurationError, std::cout);
                    }
                } else {
                    std::cerr << "No solver specified" << std::endl;
                    Words::Host::Terminate(Words::Host::ExitCode::ConfigurationError, std::cout);
                }
            } else {
                std::cerr << "System has inequalities...skipping" << std::endl;
            }
            job = jg->newJob();
        }

        if (DefinitelyNoSolutionCount == totalcount) {
            Words::Host::Terminate(Words::Host::ExitCode::DefinitelyNoSolution, std::cout);
        } else if (noSolutionCount) {
            Words::Host::Terminate(Words::Host::ExitCode::NoSolution, std::cout);
        } else {
            Words::Host::Terminate(Words::Host::ExitCode::NoIdea, std::cout);
        }
    } catch (Words::WordException &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }


}
