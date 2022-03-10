#include <chrono>
#include <iostream>
#include <queue>
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

    cout << Mxi.toString() << "\n";

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

set<pair<Words::Terminal *, int>> predecessorClosure(Automaton::NFA &Mxi, int q, map<int, set<pair<Words::Terminal *, int>>> &pred, map<int, set<int>> &epsPred, set<int> *skipped,
                                                     Words::Terminal *label = nullptr) {
    set<pair<Words::Terminal *, int>> closure;
    // Direct predecessors
    if (pred.count(q) > 0) {
        for (auto &pre : pred.at(q)) {
            if (label == nullptr || pre.first == label) {
                closure.insert(pre);
            }
        }
    }
    // Indirect predecessors
    if (epsPred.count(q) > 0) {
        for (auto &epsPre : epsPred.at(q)) {
            auto eClosure = predecessorClosure(Mxi, epsPre, pred, epsPred, skipped, label);
            skipped->insert(epsPre);
            closure.insert(eClosure.begin(), eClosure.end());
        }
    }
    cout << "\tskipped.size() = " << skipped->size() << "\n";
    return closure;
}

// Encodes: If after [i] transitions in state [q], there must be a state [q'] reachable after [i]-1 transitions and
// an edge from [q] to [q'] labeled with [filledPat[i-1]]
PLFormula AutomatonEncoder::encodePredNew(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat, map<int, set<pair<Words::Terminal *, int>>> &pred, map<int, set<int>> &epsPred) {
    vector<PLFormula> conjunctions;
    set<pair<int, int>> seen;
    queue<pair<int, int>> squeue;

    for (int qf : Mxi.getFinalStates()) {
        squeue.push(make_pair(qf, filledPat.size()));
    }

    while (!squeue.empty()) {
        auto qipair = squeue.front();
        squeue.pop();
        if (seen.count(qipair) > 0) {
            continue;
        }
        seen.insert(qipair);

        int q = qipair.first;
        int i = qipair.second;
        cout << "(" << q << ", " << i << "), remaining: " << squeue.size() << " \n";
        cout.flush();

        auto svar = -stateVars[make_pair(q, i)];

        if (i < Mxi.minReachable[q] || numTerminals(filledPat, 0, i) > Mxi.maxReachable[q]) {
            conjunctions.push_back(PLFormula::lit(svar));
            continue;
        }

        if (numTerminals(filledPat, i) > Mxi.maxrAbs[q] || filledPat.size() - i < Mxi.minrAbs[q]) {
            conjunctions.push_back(PLFormula::lit(svar));
            continue;
        }

        // Rewind until variable in pattern
        int pos = i - 1;  // position in pattern to match
        set<pair<Words::Terminal *, int>> predecessors;

        if (filledPat.at(pos).isTerminal()) {
            cout << "isTerm\n";
            cout.flush();
            predecessors = predecessorClosure(Mxi, q, pred, epsPred, new set<int>(), index2t.at(filledPat[pos].getTerminalIndex()));
            cout << boolalpha << "hier und " << filledPat.at(pos - 1).isTerminal() << "\n";
            cout.flush();
            while (filledPat.at(pos - 1).isTerminal() && pos > 0 && predecessors.size() > 0) {
                set<pair<Words::Terminal *, int>> prepredecessors{};

                cout << "|preds| = " << predecessors.size() << "\n";
                for (auto &p : predecessors) {
                    // Find predecessors of predecessor
                    set<int> *skipped = new set<int>;
                    cout << "Pre: " << p.second << "\n";
                    cout << "Address: " << &skipped << "\n";
                    for (auto &prepred : predecessorClosure(Mxi, p.second, pred, epsPred, skipped, nullptr)) {
                        // Check if edge from pre-predecessor to predecessor is viable
                        if (!prepred.first->isEpsilon() && tIndices->at(prepred.first) == filledPat.at(pos - 1).getTerminalIndex()) {
                            prepredecessors.insert(prepred);
                        }
                    }
                    seen.insert(make_pair(p.second, pos));
                    cout << "Skipped states: " << skipped->size() << "\n";
                    for (int skip : *skipped) {
                        seen.insert(make_pair(skip, pos));
                    }
                    free(skipped);
                }
                predecessors = prepredecessors;
                pos--;
            }
        } else {
            set<int> skipped;
            predecessors = predecessorClosure(Mxi, q, pred, epsPred, &skipped);
            for (int skip : skipped) {
                seen.insert(make_pair(skip, pos));
            }
        }

        vector<PLFormula> disjunctions = {PLFormula::lit(svar)};
        vector<PLFormula> conj;

        for (auto &pre : predecessors) {
            auto pvar = stateVars[make_pair(pre.second, pos)];
            int k = pre.first->isEpsilon() ? sigmaSize : tIndices->at(pre.first);
            int wordvar;
            if (filledPat.at(pos).isTerminal()) {
                int ci = filledPat[pos].getTerminalIndex();
                if (k != ci) {
                    continue;
                }
                wordvar = constantsVars->at(make_pair(ci, k));
            } else {
                pair<int, int> xij = filledPat[pos].getVarIndex();
                wordvar = variableVars->at(make_pair(xij, k));
            }
            conj.push_back(PLFormula::land({PLFormula::lit(pvar), PLFormula::lit(wordvar)}));
        }
        if (!conj.empty()) {
            disjunctions.push_back(PLFormula::lor(conj));
        }

        if (disjunctions.size() == 1) {
            conjunctions.push_back(disjunctions[0]);
        } else {
            conjunctions.push_back(PLFormula::lor(disjunctions));
        }

        if (pos > 0) {
            for (auto &pre : predecessors) {
                auto next = make_pair(pre.second, pos);
                if (seen.count(next) == 0) {
                    squeue.push(next);
                }
            }
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