#pragma once
#include <map>
#include <set>
#include <unordered_map>

#include "words/regconstraints.hpp"
#include "words/words.hpp"

namespace RegularEncoding::Automaton {

class NFA {
   private:
    std::set<int> epsilonClosure(int, std::set<int> &ignoreStates);

    int nQ = 0;
    std::map<int, std::set<std::pair<Words::Terminal *, int>>> delta;
    std::map<int, std::set<int>> deltaEpsilon;
    int initState = -1;
    std::set<int> final_states{};

   public:
    NFA() : final_states(std::set<int>{}), initState(-1){};

    /**
     * Direct instantiation.
     * @param nQ the number of numStates {0, ..., nQ-1}
     * @param delta the transition function
     * @param initState the initial state
     * @param final_states the final numStates
     */
    NFA(int nQ,
        std::map<int, std::set<std::pair<Words::Terminal *, int>>> delta,
        std::map<int, std::set<int>> deltaEpsilon,
        int initState, std::set<int> final_states)
        : nQ(nQ),
          delta(delta),
          initState(initState),
          final_states(final_states),
          deltaEpsilon(deltaEpsilon){};

    ~NFA(){};

    bool accept(const Words::Word &);

    void removeEpsilonTransitions();

    void get_reachable_states();

    NFA reduceToReachableState();

    std::set<int> getFinalStates() { return final_states; }

    int getInitialState() const { return initState; }

    std::map<int, std::set<int>> getDeltaEpsilon() {return deltaEpsilon;}

    int new_state();

    std::string toString();

    void set_initial_state(int);

    void add_final_state(int);

    void add_transition(int, Words::Terminal *, int);
    void add_eps_transition(int, int);

    int numStates() const { return nQ; }

    int numTransitions() {
        int c = 0;
        for (const auto &trans : delta) {
            c += trans.second.size();
        }
        return c;
    }

    std::map<int, std::set<std::pair<Words::Terminal *, int>>> getDelta() {
        return std::map<int, std::set<std::pair<Words::Terminal *, int>>>(
            delta);
    }
    std::unordered_map<int, int> minReachable{};
    std::unordered_map<int, int> maxReachable{};
    std::unordered_map<int, int> maxrAbs{};
    std::unordered_map<int, int> minrAbs{};

    /*
     * Returns a transition function for this automaton, where all states are
     * offset by a specific number. Required for merging NFAs without having
     * numStates clash.
     */
    std::map<int, std::vector<std::pair<Words::Terminal *, int>>> offset_delta(
        int of);
    std::map<int, std::set<int>> offset_delta_epsilon(int of);
};

/**
 * Builds an NFA out of regular expression.
 *
 * @param regExpr  the expression
 * @return an instance of an NFA, accepting the same language as the expression
 */
NFA regexToNfa(Words::RegularConstraints::RegNode &, Words::Context &);

}  // namespace RegularEncoding::Automaton