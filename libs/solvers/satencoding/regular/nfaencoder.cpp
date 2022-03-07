#include <chrono>
#include <iostream>
#include <stack>
#include <words/exceptions.hpp>

#include "encoding.h"

using namespace std;
using namespace RegularEncoding::PropositionalLogic;
using namespace chrono;

namespace RegularEncoding {

set<set<int>> AutomatonEncoder::encode() {
    auto total_start = high_resolution_clock::now();
    cout << "\n[*] Encoding ";
    constraint.toString(cout);
    cout << "\n";

    Words::Word pattern = constraint.pattern;
    shared_ptr<Words::RegularConstraints::RegNode> expr = constraint.expr;

    vector<FilledPos> filledPat = filledPattern(pattern);
    profiler.patternSize = filledPat.size();

    auto start = high_resolution_clock::now();
    Automaton::NFA M = Automaton::regexToNfa(*expr, ctx);

    // M.removeEpsilonTransitions();

    if (pattern.noVariableWord()) {
        Glucose::Var v = solver.newVar();
        if (M.accept(pattern)) {
            return set<set<int>>{set<int>{v, -v}};
        } else {
            return set<set<int>>{set<int>{v}, set<int>{-v}};
        }
    }

    // M = M.reduceToReachableState();

    if (M.getFinalStates().empty()) {
        // Does not accept anything
        Glucose::Var v = solver.newVar();
        return set<set<int>>{set<int>{v}, set<int>{-v}};
    }

    Automaton::NFA Mxi = filledAutomaton(M);

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<milliseconds>(stop - start);
    profiler.timeNFA = duration.count();
    cout << "\t - Built filled NFA with " << Mxi.numStates() << " states and " << Mxi.numTransitions() << " transitions. Took " << duration.count() << "ms\n";

    start = high_resolution_clock::now();

    // Initiate State vars
    for (int q = 0; q < Mxi.numStates(); q++) {
        for (int i = 0; i <= filledPat.size(); i++) {
            auto p = make_pair(q, i);
            stateVars[p] = solver.newVar();
        }
    }

    set<set<int>> cnf{};

    // Initial State, is a conjunction of literals
    PLFormula initialConj = encodeInitial(Mxi);
    // Add each literal as clause to cnf
    for (const auto &lit : initialConj.getSubformulae()) {
        cnf.insert(set<int>{lit.getLiteral()});
    }
    // Final States, is a disjunction of literals
    PLFormula finalDisj = encodeFinal(Mxi, filledPat);
    // Add all literals as single clause to cnf
    set<int> finalClause{};
    for (const auto &lit : finalDisj.getSubformulae()) {
        finalClause.insert(lit.getLiteral());
    }
    cnf.insert(finalClause);

    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<milliseconds>(stop - start);
    cout << "\t - Created [Final] and [Initial] constraints. Took " << duration.count() << "ms\n";

    start = high_resolution_clock::now();
    // Transition constraint, is in cnf
    PLFormula transitionCnf = encodeTransition(Mxi, filledPat);
    for (auto disj : transitionCnf.getSubformulae()) {
        set<int> clause{};
        for (const auto &lit : disj.getSubformulae()) {
            clause.insert(lit.getLiteral());
        }
        cnf.insert(clause);
    }
    if (!Mxi.getDeltaEpsilon().empty()) {
        PLFormula epsTransitionCnf = encodeEpsTransition(Mxi, filledPat);
        for (auto disj : epsTransitionCnf.getSubformulae()) {
            set<int> clause{};
            for (const auto &lit : disj.getSubformulae()) {
                clause.insert(lit.getLiteral());
            }
            cnf.insert(clause);
        }
    }

    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<milliseconds>(stop - start);
    profiler.timeFormulaTransition = duration.count();
    cout << "\t - Created [Transition] constraint. Took " << duration.count() << "ms\n";

    start = high_resolution_clock::now();
    // Predecessor constraint

    auto delta = M.getDelta();

    map<int, set<pair<Words::Terminal *, int>>> pred;
    for (auto &trans : Mxi.getDelta()) {
        int qsrc = trans.first;
        for (auto &target : trans.second) {
            Words::Terminal *label = target.first;
            int qdst = target.second;
            if (pred.count(qdst) == 1) {
                pred[qdst].insert(make_pair(label, qsrc));
            } else {
                pred[qdst] = set<pair<Words::Terminal *, int>>{make_pair(label, qsrc)};
            }
        }
    }

    map<int, set<int>> epsPred;
    for (auto &epsTrans : Mxi.getDeltaEpsilon()) {
        int qsrc = epsTrans.first;
        for (auto &qdest : epsTrans.second) {
            if (epsPred.count(qdest) == 1) {
                epsPred[qdest].insert(qsrc);
            } else {
                epsPred[qdest] = set<int>{qsrc};
            }
        }
    }

    PLFormula predecessor = encodePredNew(Mxi, filledPat, pred, epsPred);

    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<milliseconds>(stop - start);
    profiler.timeFormulaPredecessor = duration.count();
    cout << "\t - Created [Predecessor] constraint (depth: " << predecessor.depth() << ", size: " << predecessor.size() << "). Took " << duration.count() << "ms\n";

    start = high_resolution_clock::now();
    set<set<int>> predecessorCnf = tseytin_cnf(predecessor, solver);
    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<milliseconds>(stop - start);
    profiler.timeTseytinPredecessor = duration.count();
    cout << "\t - Created CNF with " << predecessorCnf.size() << " clauses. Took " << duration.count() << "ms\n";
    for (const auto &clause : predecessorCnf) {
        cnf.insert(clause);
    }

    duration = duration_cast<milliseconds>(high_resolution_clock::now() - total_start);
    cout << "[*] Encoding done. Took " << duration.count() << "ms in total" << endl;

    return cnf;
}

PLFormula AutomatonEncoder::encodeInitial(Automaton::NFA &Mxi) {
    vector<PLFormula> conj;

    set<int> q0closure = Mxi.epsilonClosure(Mxi.getInitialState());
    for (int q = 0; q < Mxi.numStates(); q++) {
        if (q0closure.count(q) == 1) {
            PLFormula init = PLFormula::lit(stateVars[make_pair(q, 0)]);
            conj.push_back(init);
        } else {
            PLFormula notInit = PLFormula::lit(-stateVars[make_pair(q, 0)]);
            conj.push_back(notInit);
        }
    }
    return PLFormula::land(conj);
}

PLFormula AutomatonEncoder::encodeFinal(Automaton::NFA &Mxi, const std::vector<FilledPos> &filledPat) {
    vector<PLFormula> disj;
    int n = (int)filledPat.size();
    for (int q : Mxi.getFinalStates()) {
        auto p = make_pair(q, n);
        disj.push_back(PLFormula::lit(stateVars[p]));
    }
    return PLFormula::lor(disj);
}

PropositionalLogic::PLFormula AutomatonEncoder::encodeEpsTransition(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat) {
    vector<PLFormula> disj;

    for (auto &trans : Mxi.getDeltaEpsilon()) {
        int src = trans.first;
        for (int i = 0; i < filledPat.size(); i++) {
            if (numTerminals(filledPat, i) > Mxi.maxrAbs[trans.first] || filledPat.size() - i < Mxi.minrAbs[trans.first]) {
                continue;
            }
            for (int dest : trans.second) {
                vector<PLFormula> clause;
                int s = stateVars[make_pair(trans.first, i)];
                int ssucc = stateVars[make_pair(dest, i)];
                clause.push_back(PLFormula::lit(-s));
                clause.push_back(PLFormula::lit(ssucc));
                disj.push_back(PLFormula::lor(clause));
            }
        }
    }
    if (disj.size() > 0) {
        return PLFormula::land(disj);
    }

    return PLFormula::land(vector<PLFormula>{ftrue});
}

PLFormula AutomatonEncoder::encodeTransition(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat) {
    vector<PLFormula> disj;

    for (auto &trans : Mxi.getDelta()) {
        for (int i = 0; i < filledPat.size(); i++) {
            if (numTerminals(filledPat, i) > Mxi.maxrAbs[trans.first] || filledPat.size() - i < Mxi.minrAbs[trans.first]) {
                continue;
            }
            for (auto &target : trans.second) {
                vector<PLFormula> clause;
                int k = target.first->isEpsilon() ? sigmaSize : tIndices->at(target.first);

                int s = stateVars[make_pair(trans.first, i)];
                int word;
                if (filledPat[i].isTerminal()) {
                    int ci = filledPat[i].getTerminalIndex();
                    word = constantsVars->at(make_pair(ci, k));
                } else {
                    pair<int, int> xij = filledPat[i].getVarIndex();
                    word = variableVars->at(make_pair(xij, k));
                }
                int ssucc = stateVars[make_pair(target.second, i + 1)];

                clause.push_back(PLFormula::lit(-s));
                clause.push_back(PLFormula::lit(-word));
                clause.push_back(PLFormula::lit(ssucc));

                disj.push_back(PLFormula::lor(clause));
            }
        }
    }

    if (disj.size() > 0) {
        return PLFormula::land(disj);
    }

    return PLFormula::land(vector<PLFormula>{ffalse});
}

// Encodes: If after [i] transitions in state [q], there must be a state [q'] reachable after [i]-1 transitions and
// an edge from [q] to [q'] labeled with [filledPat[i-1]]
PLFormula AutomatonEncoder::encodePredNew(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat, map<int, set<pair<Words::Terminal *, int>>> &pred, map<int, set<int>> &epsPred) {
    vector<PLFormula> conjunctions;
    for (int i = 1; i <= filledPat.size(); i++) {
        for (int q = 0; q < Mxi.numStates(); q++) {
            int minLength = numTerminals(filledPat, 0, i);
            int maxLength = i;
            if (maxLength < Mxi.minReachable[q] || minLength > Mxi.maxReachable[q]) {
                int succVar = -stateVars[make_pair(q, i)];
                conjunctions.push_back(PLFormula::lit(succVar));
                continue;
            }

            auto svar = -stateVars[make_pair(q, i)];
            vector<PLFormula> disjunctions = {PLFormula::lit(svar)};
            if (pred.count(q) != 0) {
                for (auto &pre : pred[q]) {
                    auto pvar = stateVars[make_pair(pre.second, i - 1)];
                    // Index of wordvar
                    int k = pre.first->isEpsilon() ? sigmaSize : tIndices->at(pre.first);
                    int wordvar;
                    if (filledPat.at(i - 1).isTerminal()) {
                        int ci = filledPat[i - 1].getTerminalIndex();
                        // TODO: What do if k != ci
                        wordvar = constantsVars->at(make_pair(ci, k));
                    } else {
                        pair<int, int> xij = filledPat[i - 1].getVarIndex();
                        wordvar = variableVars->at(make_pair(xij, k));
                    }
                    disjunctions.push_back(PLFormula::land({PLFormula::lit(pvar), PLFormula::lit(wordvar)}));
                }
            }

            if (epsPred.count(q) != 0) {
                for (int epsPre : epsPred[q]) {
                    disjunctions.push_back(PLFormula::lit(stateVars[make_pair(epsPre, i)]));
                }
            }
            conjunctions.push_back(PLFormula::lor(disjunctions));
        }
    }
    return PLFormula::land(conjunctions);
}

Automaton::NFA AutomatonEncoder::filledAutomaton(Automaton::NFA &nfa) {
    Automaton::NFA Mxi(nfa.numStates(), nfa.getDelta(), nfa.getDeltaEpsilon(), nfa.getInitialState(), nfa.getFinalStates());
    for (int q = 0; q < Mxi.numStates(); q++) {
        Mxi.add_transition(q, ctx.getEpsilon(), q);
    }
    Mxi.maxReachable = nfa.maxReachable;
    Mxi.minReachable = nfa.minReachable;
    return Mxi;
}

}  // namespace RegularEncoding