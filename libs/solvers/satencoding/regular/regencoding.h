#include <map>
#include <set>
#include <unordered_set>
#include <list>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>
#include "words/regconstraints.hpp"
#include "words/words.hpp"
#include "core/Solver.h"
#include <fstream>
#include <ctime>


namespace RegularEncoding {


    Words::RegularConstraints::RegConstraint stripPrefix(Words::RegularConstraints::RegConstraint);

    Words::RegularConstraints::RegConstraint stripSuffix(Words::RegularConstraints::RegConstraint);

    std::shared_ptr<Words::RegularConstraints::RegOperation>
    makeNodeBinary(std::shared_ptr<Words::RegularConstraints::RegOperation> concat);

    namespace Automaton {

        class NFA {
        private:
            std::set<int> epsilonClosure(int, std::set<int>& ignoreStates);

            int nQ = 0;
            std::map<int, std::set<std::pair<Words::Terminal *, int>>> delta;
            int initState = -1;
            std::set<int> final_states{};

        public:
            NFA() : final_states(std::set<int>{}), initState(-1) {};

            /**
             * Direct instantiation.
             * @param nQ the number of numStates {0, ..., nQ-1}
             * @param delta the transition function
             * @param initState the initial state
             * @param final_states the final numStates
             */
            NFA(int nQ, std::map<int, std::set<std::pair<Words::Terminal *, int>>> delta, int initState,
                std::set<int> final_states) : nQ(nQ), delta(delta), initState(initState), final_states(final_states) {};


            ~NFA() {};

            bool accept(const Words::Word&);

            void removeEpsilonTransitions();

            void get_reachable_states();

            NFA reduceToReachableState();

            std::set<int> getFinalStates() {
                return final_states;
            }

            int getInitialState() {
                return initState;
            }

            int new_state();

            std::string toString();

            void set_initial_state(int);

            void add_final_state(int);

            void add_transition(int, Words::Terminal *, int);

            int numStates() {
                return nQ;
            }

            int numTransitions() {
                int c = 0;
                for (auto trans: delta) {
                    c += trans.second.size();
                }
                return c;
            }

            std::map<int, std::set<std::pair<Words::Terminal *, int>>> getDelta() {
                return std::map<int, std::set<std::pair<Words::Terminal *, int>>>(delta);
            }


            /*
             * Returns a transition function for this automaton, where all states are offset by a specific number.
             * Required for merging NFAs without having numStates clash.
             */
            std::map<int, std::vector<std::pair<Words::Terminal *, int>>> offset_states(int of);
        };

        /**
         * Builds an NFA out of regular expression.
         *
         * @param regExpr  the expression
         * @return an instance of an NFA, accepting the same language as the expression
         */
        NFA regexToNfa(Words::RegularConstraints::RegNode &, Words::Context &);

    } // namespace Automaton

    namespace PropositionalLogic {

        enum class Junctor {
            AND = 1, OR = 2, NOT = 3, LIT = 0
        };

        class PLFormula {
        private:
            PLFormula(Junctor, std::vector<PLFormula>);

            explicit PLFormula(int); // Literal formula
            //~PLFormula();

            Junctor junctor;
            int literal;
            std::vector<PLFormula> subformulae;

        public:
            static PLFormula land(std::vector<PLFormula>);

            static PLFormula lor(std::vector<PLFormula>);

            static PLFormula lnot(PLFormula);

            static PLFormula lit(int);

            bool valid();

            bool isFalse();

            bool is_nenf();

            int depth();

            int size();

            void makeBinary();

            int max_var();

            std::string toString();

            Junctor getJunctor() { return junctor; }

            std::vector<PLFormula> getSubformulae() { return subformulae; }

            int getLiteral() const { return literal; }
        };

        class PLFormulaPtd {
        private:
            PLFormulaPtd(Junctor, std::vector<std::shared_ptr<PLFormulaPtd>>);

            explicit PLFormulaPtd(int); // Literal formula
            //~PLFormula();

            Junctor junctor;
            int literal;
            std::vector<std::shared_ptr<PLFormulaPtd>> subformulae;

        public:
            static std::shared_ptr<PLFormulaPtd> land(std::vector<std::shared_ptr<PLFormulaPtd>>);

            static std::shared_ptr<PLFormulaPtd> lor(std::vector<std::shared_ptr<PLFormulaPtd>>);

            static std::shared_ptr<PLFormulaPtd> lnot(std::shared_ptr<PLFormulaPtd>);

            static std::shared_ptr<PLFormulaPtd> lit(int);

            void makeBinary();

            static std::shared_ptr<PLFormulaPtd> fromPLF(PLFormula &f);


            int size();

            std::string toString();

            Junctor getJunctor() { return junctor; }

            std::vector<std::shared_ptr<PLFormulaPtd>> getSubformulae() { return subformulae; }

            int getLiteral() const { return literal; }
        };

        std::set<std::set<int>> tseytin_cnf(PLFormula &, Glucose::Solver &s);

    } // namespace PropositionalLogic


    namespace LengthAbstraction {

        class ArithmeticProgressions {
        public:
            ArithmeticProgressions() {
                progressions = std::set<std::pair<int, int>>{};
            }

            void add(std::pair<int, int> p) {
                //progressions.push_back(p);
                progressions.insert(p);
            };


            void mergeOther(ArithmeticProgressions &other) {
                std::set<std::pair<int, int>> newProgs;
                for (auto p: other.getProgressions()) {
                    progressions.insert(p);
                }
            }

            std::set<std::pair<int, int>> &getProgressions() {
                return progressions;
            }

            std::string toString() {
                if (progressions.size() == 0) {
                    return "{}";
                }
                std::stringstream ss;
                ss << "{";
                for (std::pair<int, int> ab: progressions) {
                    ss << "(" << ab.first << ", " << ab.second << "), ";
                }
                ss << "}";
                return ss.str();
            }

            /**
             * Returns true if and only if there is some progression (a,b) such that for an n k = a+nb holds.
             * @param k the interger to check
             */
            bool contains(int k) {
                if (k < 0) {
                    return false;
                }
                for (std::pair<int, int> p: progressions) {
                    int a = p.first;
                    int b = p.second;
                    if (b == 0) {
                        if (k == a) {
                            // k = a + 0b
                            return true;
                        }
                    } else {
                        int n = (k - a) / b;
                        if (n >= 0 && k == a + n * b) {
                            return true;
                        }
                    }
                }
                return false;
            }


        private:
            std::set<std::pair<int, int>> progressions;

        };

        ArithmeticProgressions fromExpression(Words::RegularConstraints::RegNode &);

        class UNFALengthAbstractionBuilder {
        public:
            UNFALengthAbstractionBuilder(Automaton::NFA);

            ~UNFALengthAbstractionBuilder() {};

            ArithmeticProgressions forState(int q);

            ArithmeticProgressions forStateComplete(int q);

        private:
            Automaton::NFA nfa;
            int N;
            std::vector<std::vector<bool>> adjm;
            std::vector<std::set<int>> sccs;

            std::vector<std::vector<bool>> buildAdjacencyMatrix();

            std::shared_ptr<std::set<int>> succ(std::shared_ptr<std::set<int>> &);

            std::shared_ptr<std::set<int>> pre(std::shared_ptr<std::set<int>> &);

            std::vector<std::shared_ptr<std::set<int>>> T;
            std::vector<std::vector<std::unordered_set<int>>> Sq;
            std::map<std::set<int>, std::shared_ptr<std::set<int>>> successorsCache{};
            std::map<std::set<int>, std::shared_ptr<std::set<int>>> predecessorCache{};
            std::map<int, std::shared_ptr<ArithmeticProgressions>> statewiserAbs;

            /**
             * Shortest loop from q to q in the graph
             * Ignored can be a set of nodes to ignore during the search.
             * This is useful if the SCC of q is known, because then ignored should contain all nodes not in the SCC of q.
             * Nodes not in the SCC can not be within in a loop starting and ending in q.
             */
            int sl(int q, const std::set<int>& ignore);

        };
        

    }


    class FilledPos {
    public:
        FilledPos(int tIndex) : tIndex(tIndex), isTerm(true) {};

        FilledPos(std::pair<int, int> vIndex) : vIndex(std::move(vIndex)), isTerm(false) {};

        bool isTerminal() {
            return isTerm;
        }

        bool isVariable() {
            return !isTerm;
        }

        int getTerminalIndex() {
            return tIndex;
        }

        std::pair<int, int> getVarIndex() {
            return vIndex;
        }


    private:
        bool isTerm;
        int tIndex = -1;
        std::pair<int, int> vIndex = std::make_pair(-1, -1);
    };




    struct InductiveProfiler {
        int patternSize;
        unsigned long timeLengthAbstraction;
        unsigned long skipped;
        unsigned long timeFormula;
        unsigned long timeTseytin;
    };

    struct AutomatonProfiler {
        int patternSize;
        unsigned long timeNFA;
        unsigned long timeLengthAbstraction;
        unsigned long timeFormulaTransition;
        unsigned long timeFormulaPredecessor;
        unsigned long timeTseytinPredecessor;
    };

    struct EncodingProfiler {
        int bound;
        int exprComplexity;
        int depth;
        int longestLiteral;
        int shortestLiteral;
        int numStars;
        int starHeight;
        unsigned long timeEncoding;
        unsigned long timeSolving;
        unsigned long timeTotal;
        bool sat;
        bool automaton;
        AutomatonProfiler automatonProfiler;
        InductiveProfiler inductiveProfiler;
    };



    class Encoder {
    public:
        Encoder(Words::RegularConstraints::RegConstraint constraint,
                Words::Context ctx,
                Glucose::Solver &solver,
                int sigmaSize,
                std::map<Words::Variable *, int> *vIndices,
                std::map<int, int> *maxPadding,
                std::map<Words::Terminal *, int> *tIndices,
                std::map<std::pair<std::pair<int, int>, int>, Glucose::Var> *variableVars,
                std::map<std::pair<int, int>, Glucose::Var> *constantsVars,
                std::map<int, Words::Terminal *> &index2t) :
                constraint(std::move(constraint)), ctx(ctx), vIndices(vIndices), tIndices(tIndices),
                index2t(index2t),
                maxPadding(maxPadding),
                variableVars(variableVars), constantsVars(constantsVars), solver(solver), sigmaSize(sigmaSize),
                ffalse(PropositionalLogic::PLFormula::lit(0)), ftrue(PropositionalLogic::PLFormula::lit(0)) {

            Glucose::Var falseVar = solver.newVar();
            ffalse = PropositionalLogic::PLFormula::land(
                    std::vector<PropositionalLogic::PLFormula>{PropositionalLogic::PLFormula::lit(falseVar),
                                                               PropositionalLogic::PLFormula::lit(-falseVar)});

            Glucose::Var trueVar = solver.newVar();
            ftrue = PropositionalLogic::PLFormula::lor(
                    std::vector<PropositionalLogic::PLFormula>{PropositionalLogic::PLFormula::lit(trueVar),
                                                               PropositionalLogic::PLFormula::lit(-trueVar)});
        };

        virtual ~Encoder() {};

        virtual std::set<std::set<int>> encode() {};

        std::vector<FilledPos> filledPattern(const Words::Word&);

    protected:
        Words::RegularConstraints::RegConstraint constraint;
        Words::Context ctx;
        std::map<Words::Variable *, int> *vIndices;
        std::map<Words::Terminal *, int> *tIndices;
        std::map<int, int> *maxPadding;
        std::map<std::pair<std::pair<int, int>, int>, Glucose::Var> *variableVars;
        std::map<std::pair<int, int>, Glucose::Var> *constantsVars;
        Glucose::Solver &solver;
        int sigmaSize;
        std::map<int, Words::Terminal *> &index2t;

        PropositionalLogic::PLFormula ffalse;
        PropositionalLogic::PLFormula ftrue;

    };

    class InductiveEncoder : public Encoder {
    public:
        InductiveEncoder(Words::RegularConstraints::RegConstraint constraint,
                         Words::Context ctx,
                         Glucose::Solver &solver,
                         int sigmaSize,
                         std::map<Words::Variable *, int> *vIndices,
                         std::map<int, int> *maxPadding,
                         std::map<Words::Terminal *, int> *tIndices,
                         std::map<std::pair<std::pair<int, int>, int>, Glucose::Var> *variableVars,
                         std::map<std::pair<int, int>, Glucose::Var> *constantsVars,
                         std::map<int, Words::Terminal *> &index2t,
                         InductiveProfiler& profiler)
                : Encoder(constraint, ctx, solver, sigmaSize, vIndices, maxPadding, tIndices, variableVars,
                          constantsVars, index2t), profiler(profiler) {

        };

        std::set<std::set<int>> encode();

    private:
        PropositionalLogic::PLFormula
        doEncode(const std::vector<FilledPos>&, const std::shared_ptr<Words::RegularConstraints::RegNode>& expression);

        PropositionalLogic::PLFormula
        encodeWord(std::vector<FilledPos> filledPat, std::vector<int> expressionIdx);

        PropositionalLogic::PLFormula
        encodeUnion(std::vector<FilledPos> filledPat,
                    const std::shared_ptr<Words::RegularConstraints::RegOperation>& expression);

        PropositionalLogic::PLFormula
        encodeConcat(std::vector<FilledPos> filledPat,
                     const std::shared_ptr<Words::RegularConstraints::RegOperation>& expression);

        PropositionalLogic::PLFormula
        encodeStar(std::vector<FilledPos> filledPat,
                   const std::shared_ptr<Words::RegularConstraints::RegOperation>& expression);

        PropositionalLogic::PLFormula
        encodeNone(const std::vector<FilledPos>& filledPat,
                   const std::shared_ptr<Words::RegularConstraints::RegEmpty>& empty);

        //std::unordered_map<size_t, PropositionalLogic::PLFormula*> ccache;
        InductiveProfiler& profiler;

    };

    class AutomatonEncoder : public Encoder {
    public:

        AutomatonEncoder(Words::RegularConstraints::RegConstraint constraint,
                         Words::Context ctx,
                         Glucose::Solver &solver,
                         int sigmaSize,
                         std::map<Words::Variable *, int> *vIndices,
                         std::map<int, int> *maxPadding,
                         std::map<Words::Terminal *, int> *tIndices,
                         std::map<std::pair<std::pair<int, int>, int>, Glucose::Var> *variableVars,
                         std::map<std::pair<int, int>, Glucose::Var> *constantsVars,
                         std::map<int, Words::Terminal *> &index2t,
                         AutomatonProfiler& profiler)
                : Encoder(constraint, ctx, solver, sigmaSize, vIndices, maxPadding, tIndices, variableVars,
                          constantsVars, index2t), profiler(profiler) {

        };

        std::set<std::set<int>> encode();

    private:
        PropositionalLogic::PLFormula encodeTransition(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat);

        PropositionalLogic::PLFormula encodePredecessor(Automaton::NFA &nfa, std::vector<FilledPos> filledPat);

        PropositionalLogic::PLFormula encodeFinal(Automaton::NFA &nfa, const std::vector<FilledPos>& filledPat);

        PropositionalLogic::PLFormula encodeInitial(Automaton::NFA &nfa);

        PropositionalLogic::PLFormula encodePredNew(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat, int q, int i,
                                                    std::set<std::pair<int, int>> &,
                                                    std::map<int, std::set<std::pair<Words::Terminal *, int>>> &);

        Automaton::NFA filledAutomaton(Automaton::NFA &nfa);

        std::map<std::pair<int, int>, int> stateVars{};

        std::unordered_map<int, std::shared_ptr<LengthAbstraction::ArithmeticProgressions>> satewiseLengthAbstraction{};

        std::map<int, std::shared_ptr<std::set<int>>> reachable;

        AutomatonProfiler& profiler;

    };

} // namespace RegularEncoding