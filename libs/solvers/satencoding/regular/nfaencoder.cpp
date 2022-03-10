#include <iostream>
#include <words/exceptions.hpp>
#include "encoding.h"
#include <chrono>
#include <stack>


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

        M.removeEpsilonTransitions();


        if (pattern.noVariableWord()) {
            Glucose::Var v = solver.newVar();
            if (M.accept(pattern)) {
                return set<set<int>>{set<int>{v, -v}};
            } else {
                return set<set<int>>{set<int>{v}, set<int>{-v}};
            }
        }

        M = M.reduceToReachableState();

        if (M.getFinalStates().empty()) {
            // Does not accept anything
            Glucose::Var v = solver.newVar();
            return set<set<int>>{set<int>{v}, set<int>{-v}};
        }


        Automaton::NFA Mxi = filledAutomaton(M);


        auto stop = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<milliseconds>(stop - start);
        profiler.timeNFA = duration.count();
        cout << "\t - Built filled NFA with " << Mxi.numStates() << " states and " << Mxi.numTransitions()
             << " transitions. Took " << duration.count() << "ms\n";



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
        for (const auto &lit: initialConj.getSubformulae()) {
            cnf.insert(set<int>{lit.getLiteral()});
        }
        // Final States, is a disjunction of literals
        PLFormula finalDisj = encodeFinal(Mxi, filledPat);
        // Add all literals as single clause to cnf
        set<int> finalClause{};
        for (const auto &lit: finalDisj.getSubformulae()) {
            finalClause.insert(lit.getLiteral());
        }
        cnf.insert(finalClause);

        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        cout << "\t - Created [Final] and [Initial] constraints. Took " << duration.count() << "ms\n";

        
        start = high_resolution_clock::now();
        // Transition constraint, is in cnf
        PLFormula transitionCnf = encodeTransition(Mxi, filledPat);
        
        for (auto disj: transitionCnf.getSubformulae()) {
            set<int> clause{};
            for (const auto &lit: disj.getSubformulae()) {
                clause.insert(lit.getLiteral());
            }
            cnf.insert(clause);
        }
        

        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        profiler.timeFormulaTransition = duration.count();
        cout << "\t - Created [Transition] constraint. Took " << duration.count() << "ms\n";

        start = high_resolution_clock::now();
        // Predecessor constraint

        auto delta = M.getDelta();



        map<int, set<pair<Words::Terminal *, int>>> pred;
        for (auto &trans: Mxi.getDelta()) {
            int qsrc = trans.first;
            for (auto &target: trans.second) {
                Words::Terminal *label = target.first;
                int qdst = target.second;
                if (pred.count(qdst) == 1) {
                    pred[qdst].insert(make_pair(label, qsrc));
                } else {
                    pred[qdst] = set<pair<Words::Terminal *, int>>{make_pair(label, qsrc)};
                }
            }
        }


        set<pair<int, int>> tmpv{};
        vector<PLFormula> predFormulae;
        for (auto qf: Mxi.getFinalStates()) {
            PLFormula predPart = encodePredNew(Mxi, filledPat, qf, (int) filledPat.size(), tmpv, pred);

            predFormulae.push_back(predPart);

        }
        PLFormula predecessor = PLFormula::land(predFormulae);
        if (predFormulae.size() == 1) {
            predecessor = predFormulae[0];
        }

        // OLD ITERATIVE APPROACH
        // PLFormula predecessor = encodePredecessor(Mxi, filledPat);


        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        profiler.timeFormulaPredecessor = duration.count();
        cout << "\t - Created [Predecessor] constraint (depth: " << predecessor.depth() << ", size: "
             << predecessor.size() << "). Took " << duration.count() << "ms\n";

        start = high_resolution_clock::now();
        set<set<int>> predecessorCnf = tseytin_cnf(predecessor, solver);
        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        profiler.timeTseytinPredecessor = duration.count();
        cout << "\t - Created CNF. Took " << duration.count() << "ms\n";
        for (const auto &clause: predecessorCnf) {
            cnf.insert(clause);
        }


        duration = duration_cast<milliseconds>(high_resolution_clock::now() - total_start);
        cout << "[*] Encoding done. Took " << duration.count() << "ms in total" << endl;


        return cnf;

    }

    PLFormula AutomatonEncoder::encodeInitial(Automaton::NFA &Mxi) {
        vector<PLFormula> conj;
        PLFormula init = PLFormula::lit(stateVars[make_pair(Mxi.getInitialState(), 0)]);
        conj.push_back(init);
        for (int q = 1; q < Mxi.numStates(); q++) {
            PLFormula notInit = PLFormula::lit(-stateVars[make_pair(q, 0)]);
            conj.push_back(notInit);
        }
        return PLFormula::land(conj);
    }

    PLFormula AutomatonEncoder::encodeFinal(Automaton::NFA &Mxi, const std::vector<FilledPos> &filledPat) {
        vector<PLFormula> disj;
        int n = (int) filledPat.size();
        for (int q: Mxi.getFinalStates()) {
            auto p = make_pair(q, n);
            disj.push_back(PLFormula::lit(stateVars[p]));
        }
        return PLFormula::lor(disj);
    }

    PLFormula AutomatonEncoder::encodeTransition(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat) {
        vector<PLFormula> disj;
        
        for (auto &trans: Mxi.getDelta()) {
            for (int i = 0; i < filledPat.size(); i++) {
                if (numTerminals(filledPat, i) > Mxi.maxrAbs[trans.first] || filledPat.size() - i < Mxi.minrAbs[trans.first]) {
                    //int succVar = -stateVars[make_pair(trans.first, i)];
                    //disj.push_back(PLFormula::lit(succVar));
                    continue;
                }
                for (auto &target: trans.second) {


                    vector<PLFormula> clause;

                    

                    int k;
                    if (target.first->isEpsilon()) {
                        k = sigmaSize;
                    } else {
                        k = tIndices->at(target.first);
                    }

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
            return PLFormula::land(disj);
        }

        return PLFormula::land(vector<PLFormula>{ffalse});
    }



    bool checkLength(int q, int q0, map<int, set<pair<Words::Terminal *, int>>> &pred, int minLength, int maxLenght) {
        std::stack<std::pair<int, int>> todo{};
        todo.push(std::make_pair(q, maxLenght));
        std::set<std::pair<int, int>> seen{};
        while (!todo.empty()) {
            auto current = todo.top();
            todo.pop();
            seen.insert(current);
            int cq = std::get<0>(current);
            int ci = std::get<1>(current);
            if (cq == q0) {
                if (minLength <= ci) {
                    return true;
                }
            } else {
                if (minLength < ci) {
                    for (auto& tran: pred[cq]) {
                        if (!tran.first->isEpsilon()) {
                            std::pair<int, int> next = std::make_pair(tran.second, ci-1);
                            if (seen.find(next) == seen.end()) {
                                todo.push(next);
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    /**
     * Deprecated, use AutomatonEncoder::encodePredNew
     */
    PLFormula AutomatonEncoder::encodePredecessor(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat) {
        // calc pred
        map<int, set<pair<Words::Terminal *, int>>> pred;
        for (auto &trans: Mxi.getDelta()) {
            int qsrc = trans.first;
            for (auto &target: trans.second) {
                Words::Terminal *label = target.first;
                int qdst = target.second;
                if (pred.count(qdst) == 1) {
                    pred[qdst].insert(make_pair(label, qsrc));
                } else {
                    pred[qdst] = set<pair<Words::Terminal *, int>>{make_pair(label, qsrc)};
                }
            }
        }

        set<pair<int, int>> visited{};


        vector<PLFormula> disj;
        for (int i = filledPat.size(); i >= 1; i--) {
            for (int q = Mxi.numStates() - 1; q >= 0; q--) {

                if (visited.count(make_pair(q, i))) {
                    continue;
                }

                int currentPos = i - 1; // Current position in pattern
                set<pair<Words::Terminal *, int>> preds{}; // Predecessors of q
                for (auto t: pred[q]) {
                    if (filledPat.at(currentPos).isTerminal()) {
                        if (!t.first->isEpsilon() &&
                            tIndices->at(t.first) == filledPat[currentPos].getTerminalIndex()) {
                            preds.insert(t);
                        }
                    } else {
                        preds.insert(t);
                    }
                }

                // Go back while reading constants in the pattern from right to left
                while (currentPos - 1 >= 1 && filledPat.at(currentPos - 1).isTerminal()) {
                    set<pair<Words::Terminal *, int>> predPreds{};
                    for (auto p: preds) {
                        for (auto pp: pred[p.second]) { // Predecessors of p
                            if (!pp.first->isEpsilon() &&
                                tIndices->at(pp.first) == filledPat.at(currentPos - 1).getTerminalIndex()) {
                                predPreds.insert(pp);

                            } else if (pp.first->isEpsilon()) {
                                predPreds.insert(pp);
                            }
                        }
                        visited.insert(make_pair(p.second, currentPos));
                    }
                    preds = predPreds;
                    currentPos--;
                }

                vector<PLFormula> pred_q_disj; // S_q^i -> (\/ {(S_pr^i /\ w)})
                PLFormula sqi = PLFormula::lit(-stateVars[make_pair(q, i)]); // S_q^i
                pred_q_disj.push_back(sqi);
                if (preds.size() > 0) {
                    // Has predecessor(s)
                    for (auto q_pred: preds) {

                        PLFormula sqip = PLFormula::lit(stateVars[make_pair(q_pred.second, currentPos)]); // S_q'^i

                        vector<PLFormula> conj;
                        int word;
                        int k;
                        if (q_pred.first->isEpsilon()) {
                            k = sigmaSize;
                        } else {
                            k = tIndices->at(q_pred.first);
                        }
                        if (filledPat[currentPos].isTerminal()) {
                            int ci = filledPat[currentPos].getTerminalIndex();
                            if (ci != k) {
                                // UNSAT!
                                continue;
                            }
                            word = constantsVars->at(make_pair(ci, k));
                        } else {
                            pair<int, int> xij = filledPat[currentPos].getVarIndex();
                            word = variableVars->at(make_pair(xij, k));
                        }
                        conj.push_back(sqip);
                        conj.push_back(PLFormula::lit(word));

                        pred_q_disj.push_back(PLFormula::land(conj));
                    }
                    disj.push_back(PLFormula::lor(pred_q_disj));
                } else {
                    disj.push_back(sqi);
                }
            }
        }

        if (disj.empty()) {
            // TODO: FALSE or TRUE?
            return ffalse;
        } else if (disj.size() == 1) {
            return disj[0];
        } else {
            return PLFormula::land(disj);
        }
    }


    // Encodes: If after [i] transitions in state [q], there must be a state [q'] reachable after [i]-1 transitions and
    // an edge from [q] to [q'] labeled with [filledPat[i-1]]
    PLFormula AutomatonEncoder::encodePredNew(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat, int q, int i,
                                              set<pair<int, int>> &visited,
                                              map<int, set<pair<Words::Terminal *, int>>> &pred) {


        visited.insert(make_pair(q, i));
        vector<PLFormula> f{};


        // Check whether there is path back to q0 at least covering the terminals and at most the pattern up to position i.
        int minLength = numTerminals(filledPat, 0, i);
        int maxLength = i;
        if (maxLength < Mxi.minReachable[q]  || minLength > Mxi.maxReachable[q] ) {
            // cout << "No path with length in [" << minLength << ", " << maxLength << "] from q0 to state " << q;
            // cout << "(is [" << Mxi.minReachable[q] << ", " << Mxi.maxReachable[q] << "])\n";
            int succVar = -stateVars[make_pair(q, i)];
            return PLFormula::lit(succVar);
        }

        int currentPos = i - 1; // Position of i in the pattern

        // Find all predecessors q' of q where there is an edge labeled with filledPat[i-1] (always the case if filledPat[i-1] is variable)
        set<pair<Words::Terminal *, int>> preds{};
        for (auto t: pred[q]) {
            if (filledPat.at(currentPos).isTerminal()) {
                if (!t.first->isEpsilon() && tIndices->at(t.first) == filledPat[currentPos].getTerminalIndex()) {
                    preds.insert(t);
                }
            } else {
                preds.insert(t);
            }
        }

        // Go back while reading constants in the pattern from right to left
        while (filledPat[i - 1].isTerminal() && currentPos - 1 >= 0 && filledPat[currentPos - 1].isTerminal()) {
            set<pair<Words::Terminal *, int>> predPreds{};
            for (auto p: preds) {
                for (auto pp: pred[p.second]) {
                    if ((!pp.first->isEpsilon() &&
                         tIndices->at(pp.first) == filledPat.at(currentPos - 1).getTerminalIndex())) {
                        predPreds.insert(pp);

                    }
                }
                visited.insert(make_pair(p.second, currentPos));
            }
            preds = predPreds;
            currentPos--;
        }

        // If preds is empty, we know that Sqi has no valid predecessor and we'll encode -Sqi.
        vector<PLFormula> disj;
        // For all reachable states q' in succ, encode that q is the predecessor after reading i symbols
        auto delta = Mxi.getDelta();
        vector<PLFormula> conj;
        // Predecessor Sq^i
        int succVar = -stateVars[make_pair(q, i)];
        disj.push_back(PLFormula::lit(succVar));

        for (auto qh: preds) {
            int predVar = stateVars[make_pair(qh.second, currentPos)];
            // word
            int k;
            if (qh.first->isEpsilon()) {
                k = sigmaSize;
            } else {
                k = tIndices->at(qh.first);
            }
            int word;
            if (filledPat.at(currentPos).isTerminal()) {
                int ci = filledPat[currentPos].getTerminalIndex();
                word = constantsVars->at(make_pair(ci, k));
            } else {
                pair<int, int> xij = filledPat[currentPos].getVarIndex();
                word = variableVars->at(make_pair(xij, k));
            }
            auto predF = PLFormula::land(vector<PLFormula>{PLFormula::lit(word), PLFormula::lit(predVar)});
            conj.push_back(predF);
        }

        if (!conj.empty()) {
            disj.push_back(PLFormula::lor(conj));
        }

        if (disj.size() == 1) {
            // -S
            f.push_back(disj[0]);
        } else {
            // -S \/ (... /\ ...) ...
            f.push_back(PLFormula::lor(disj));
        }

        if (currentPos > 0) {
            for (auto qh: preds) {
                if (visited.count(make_pair(qh.second, currentPos)) == 0) {
                    //Call recursively for preds
                    f.push_back(encodePredNew(Mxi, filledPat, qh.second, currentPos, visited, pred));
                }
            }
        }
        if (f.empty()) {
            return ffalse;
        } else if (f.size() == 1) {
            return f[0];
        }
        return PLFormula::land(f);
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

}