#pragma once
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
#include "nfa.h"

namespace RegularEncoding {

    


    std::shared_ptr<Words::RegularConstraints::RegOperation>
    makeNodeBinary(std::shared_ptr<Words::RegularConstraints::RegOperation> concat);

    

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



    
} // namespace RegularEncoding