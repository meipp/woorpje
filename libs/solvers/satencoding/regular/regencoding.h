#include <map>
#include <set>
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


namespace RegularEncoding {

    namespace Automaton {

        class NFA {
        private:
            std::vector<int> epsilon_closure(int);

            std::vector<int> states;
            std::map<int, std::tuple<std::string, int>> delta;
            int q0;
            std::vector<int> final_states;

        public:
            NFA(/* args */);

            ~NFA();

            bool accept(std::string);

            void remove_epsilon_transitions();

            void get_reachable_states();

            int new_state();

            void set_initial_state(int);

            void add_final_state(int);

            void add_transition(int, std::string, int);

            void remove_transition(int, std::string, int);

            std::map<int, int> merge_automaton(NFA);
        };

        NFA regex_to_nfa(std::string);

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

            bool is_nenf();

            int depth();

            void make_binary();

            int max_var();

            std::string toString();

            Junctor getJunctor() { return junctor; }

            std::vector<PLFormula> getSubformulae() { return subformulae; }

            int getLiteral() const { return literal; }
        };

        std::set<std::set<int>> tseytin_cnf(PLFormula &, Glucose::Solver &s);

    } // namespace PropositionalLogic


    namespace LengthAbstraction {

        class ArithmeticProgressions {
        public:
            ArithmeticProgressions() {
                progressions = std::list<std::pair<int, int>>{};
            }

            void add(std::pair<int, int> p) {
                progressions.push_back(p);
            };


            void mergeOther(ArithmeticProgressions& other) {
                progressions.splice(progressions.end(), other.progressions);
            }

            std::list<std::pair<int, int>>& getProgressions() {
                return progressions;
            }

            std::string toString() {
                if (progressions.size() == 0) {
                    return "{}";
                }
                std::stringstream ss;
                ss << "{";
                for (std::pair<int, int> ab: progressions) {
                     ss << "("<< ab.first << ", " << ab.second << "), ";
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
                        if (k == a + n * b) {
                            return true;
                        }
                    }
                }
                return false;
            }

            ArithmeticProgressions intersection(ArithmeticProgressions other);

        private:
            std::list<std::pair<int, int>> progressions;

        };

        // Todo: return unique_ptr for performance!
        ArithmeticProgressions fromExpression(Words::RegularConstraints::RegNode &);

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
                std::map<std::pair<int, int>, Glucose::Var> *constantsVars) :
                constraint(std::move(constraint)), ctx(ctx), vIndices(vIndices), tIndices(tIndices),
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

        std::vector<FilledPos> filledPattern(Words::Word);

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
                         std::map<std::pair<int, int>, Glucose::Var> *constantsVars)
                : Encoder(constraint, ctx, solver, sigmaSize, vIndices, maxPadding, tIndices, variableVars,
                          constantsVars) /*, *ccache(std::unordered_map<size_t, PropositionalLogic::PLFormula*>())*/ {

        };

        std::set<std::set<int>> encode();

    private:
        PropositionalLogic::PLFormula
        doEncode(std::vector<FilledPos>, std::shared_ptr<Words::RegularConstraints::RegNode> expression);

        PropositionalLogic::PLFormula
        encodeWord(std::vector<FilledPos> filledPat, std::vector<int> expressionIdx);

        PropositionalLogic::PLFormula
        encodeUnion(std::vector<FilledPos> filledPat,
                    std::shared_ptr<Words::RegularConstraints::RegOperation> expression);

        PropositionalLogic::PLFormula
        encodeConcat(std::vector<FilledPos> filledPat,
                     std::shared_ptr<Words::RegularConstraints::RegOperation> expression);

        PropositionalLogic::PLFormula
        encodeStar(std::vector<FilledPos> filledPat,
                   std::shared_ptr<Words::RegularConstraints::RegOperation> expression);

        //std::unordered_map<size_t, PropositionalLogic::PLFormula*> ccache;

    };

    class AutomatonEncoder : public Encoder {
        using Encoder::Encoder;
    };

} // namespace RegularEncoding