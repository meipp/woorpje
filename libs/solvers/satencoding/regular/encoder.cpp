#include <iostream>
#include <words/exceptions.hpp>
#include "regencoding.h"
#include <chrono>
#include <solvers/solvers.hpp>
#include <omp.h>


using namespace std;
using namespace RegularEncoding::PropositionalLogic;
using namespace chrono;


namespace RegularEncoding {

    namespace {

        int numTerminals(vector<FilledPos> &pat, int from = 0) {
            int ts = 0;
            for (int i = from; i < pat.size(); i++) {
                if (pat[i].isTerminal()) {
                    ts++;
                }
            }
            return ts;
        }

    }

    /**
     * Creates a filled pattern out of a word (containing variables).
     * Each variable x is substituted by maxPadding(x) pairs.
     * @param pattern the pattern
     * @return a vector containing the indices of the filled pattern
     */
    vector<FilledPos> Encoder::filledPattern(Words::Word pattern) {
        vector<FilledPos> filledPattern{};

        for (auto s: pattern) {
            if (s->isTerminal()) {
                int ci = tIndices->at(s->getTerminal());
                FilledPos tidx(ci);
                filledPattern.push_back(tidx);
            } else if (s->isVariable()) {
                int xi = vIndices->at(s->getVariable());
                for (int j = 0; j < maxPadding->at(xi); j++) {
                    FilledPos vidx(make_pair(xi, j));
                    filledPattern.push_back(vidx);
                }
            }
        }

        return filledPattern;

    }


    /*********************************************************************
     ************************* Inductive Encoder *************************
     *********************************************************************/

    set<set<int>> InductiveEncoder::encode() {

        cout << "\n[*] Encoding ";
        constraint.toString(cout);
        cout << "\n";

        Words::Word pattern = constraint.pattern;
        shared_ptr<Words::RegularConstraints::RegNode> expr = constraint.expr;

        vector<FilledPos> filledPat = filledPattern(pattern);

        auto start = high_resolution_clock::now();
        PLFormula f = doEncode(filledPat, expr);

        cout << "\t - Built formula (depth: " << f.depth() << ", size:" << f.size() << ") creating CNF\n";

        set<set<int>> cnf = tseytin_cnf(f, solver);
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        cout << "\t - CNF done, " << cnf.size() << " clauses\n";
        cout << "[*] Encoding done. Took " << duration.count() << "ms" << endl;
        return cnf;
    }

    PLFormula
    InductiveEncoder::doEncode(vector<FilledPos> filledPat,
                               shared_ptr<Words::RegularConstraints::RegNode> expression) {


        shared_ptr<Words::RegularConstraints::RegWord> word = dynamic_pointer_cast<Words::RegularConstraints::RegWord>(
                expression);
        shared_ptr<Words::RegularConstraints::RegOperation> opr = dynamic_pointer_cast<Words::RegularConstraints::RegOperation>(
                expression);
        shared_ptr<Words::RegularConstraints::RegEmpty> emps = dynamic_pointer_cast<Words::RegularConstraints::RegEmpty>(
                expression);

        bool isConst = true;
        for (auto fp: filledPat) {
            if (fp.isVariable()) {
                isConst = false;
                break;
            }
        }
        if (isConst) {
            auto nfa = Automaton::regexToNfa(*expression, ctx);
            Words::Word constW;
            unique_ptr<Words::WordBuilder> wb = ctx.makeWordBuilder(constW);
            for (auto fp: filledPat) {
                *wb << index2t[fp.getTerminalIndex()]->getChar();
            }
            wb->flush();
            if (nfa.accept(constW)) {
                return ftrue;
            } else {
                return ffalse;
            }
        }

        if (word != nullptr) {

            vector<int> expressionIdx{};
            for (auto s: word->word) {
                if (s->isTerminal()) {
                    expressionIdx.push_back(tIndices->at(s->getTerminal()));
                } else {
                    throw Words::WordException("Only constant regular expressions allowed");
                }
            }
            PLFormula result = encodeWord(filledPat, expressionIdx);
            return result;

        } else if (opr != nullptr) {
            switch (opr->getOperator()) {
                case Words::RegularConstraints::RegularOperator::CONCAT: {
                    return encodeConcat(filledPat, opr);
                    break;
                }
                case Words::RegularConstraints::RegularOperator::UNION: {
                    return encodeUnion(filledPat, opr);
                }
                case Words::RegularConstraints::RegularOperator::STAR: {
                    return encodeStar(filledPat, opr);
                    break;
                }
            }
        } else if (emps != nullptr) {
            return encodeNone(filledPat, emps);
        } else {
            throw Words::WordException("Invalid type");
        }

        return PLFormula::lit(0);
    }

    PLFormula
    InductiveEncoder::encodeUnion(vector<FilledPos> filledPat,
                                  shared_ptr<Words::RegularConstraints::RegOperation> expression) {
        vector<PLFormula> disj{};
        LengthAbstraction::ArithmeticProgressions la = LengthAbstraction::fromExpression(*expression);
        for (auto c: expression->getChildren()) {
            // TODO: Check length abstraction
            bool lengthOk = false;
            // < or <= ?? Also, for other cases?
            for (int i = numTerminals(filledPat); i <= filledPat.size(); i++) {
                if (la.contains(i)) {
                    lengthOk = true;
                    break;
                }
            }
            if (lengthOk) {
                PLFormula f = doEncode(filledPat, c);
                disj.push_back(f);
            }
        }
        if (disj.empty()) {
            return ffalse;
        } else if (disj.size() == 1) {
            return disj[0];
        }
        return PLFormula::lor(disj);
    }


    /**
     * Helper function to make n-ary concatenation binary.
     * Does not mutate the input parameter.
     * @param concat the n-ary concatenation
     * @return A new RegNode that has two exactly children, where the RHS contains all other concats,
     *      or the input param itself, if it was already binary.
     */
    shared_ptr<Words::RegularConstraints::RegOperation>
    makeNodeBinary(shared_ptr<Words::RegularConstraints::RegOperation> concat) {
        if (concat->getChildren().size() <= 2) {
            return concat;
        }

        // New root
        shared_ptr<Words::RegularConstraints::RegOperation> root = make_shared<Words::RegularConstraints::RegOperation>(
                Words::RegularConstraints::RegularOperator::CONCAT);

        auto lhs = concat->getChildren()[0];
        shared_ptr<Words::RegularConstraints::RegOperation> rhs = make_shared<Words::RegularConstraints::RegOperation>(
                Words::RegularConstraints::RegularOperator::CONCAT);
        for (int i = 1; i < concat->getChildren().size(); i++) {
            rhs->addChild(concat->getChildren()[i]);
        }

        // Add new children
        root->addChild(lhs);
        root->addChild(rhs);

        return root;
    }

    PLFormula
    InductiveEncoder::encodeConcat(vector<FilledPos> filledPat,
                                   shared_ptr<Words::RegularConstraints::RegOperation> expression) {


        if (expression->getChildren().size() > 2) {
            auto bin = makeNodeBinary(expression);
            return encodeConcat(filledPat, bin);
        }


        vector<PLFormula> disj{};
        auto L = expression->getChildren()[0];
        auto R = expression->getChildren()[1];


        LengthAbstraction::ArithmeticProgressions laL = LengthAbstraction::fromExpression(*L);
        LengthAbstraction::ArithmeticProgressions laR = LengthAbstraction::fromExpression(*R);


        for (int i = 0; i <= int(filledPat.size()); i++) {

            vector<FilledPos> prefix(filledPat.begin(), filledPat.begin() + i);
            vector<FilledPos> suffix(filledPat.begin() + i, filledPat.end());


            // Check length abstractions
            bool lengthOk = false;

            for (int i = numTerminals(prefix); i <= prefix.size(); i++) {
                if (laL.contains(i)) {
                    lengthOk = true;
                    break;
                }
            }

            if (!lengthOk) {
                continue;
            }
            lengthOk = false;
            for (int i = numTerminals(suffix); i <= suffix.size(); i++) {
                if (laR.contains(i)) {
                    lengthOk = true;
                    break;
                }
            }

            if (!lengthOk) {
                continue;
            }

            //cout << "\tSuffix: " << suffix.size() << "\n";
            PLFormula fl = doEncode(prefix, L);
            PLFormula fr = doEncode(suffix, R);

            PLFormula current = PLFormula::land(vector<PLFormula>{fl, fr});
            disj.push_back(current);
        }

        if (disj.empty()) {
            // None is satisfiable due to length abstraction
            return ffalse;
        } else if (disj.size() == 1) {
            return disj[0];
        } else {
            return PLFormula::lor(disj);
        }
    }

    PLFormula
    InductiveEncoder::encodeStar(vector<FilledPos> filledPat,
                                 shared_ptr<Words::RegularConstraints::RegOperation> expression) {

        LengthAbstraction::ArithmeticProgressions la = LengthAbstraction::fromExpression(*expression);

        bool lengthOk = false;
        // < or <= ?? Also, for other cases?
        for (int i = numTerminals(filledPat); i <= filledPat.size(); i++) {
            if (la.contains(i)) {
                lengthOk = true;
                break;
            }
        }

        if (!lengthOk) {
            return ffalse;
        }

        vector<PLFormula> disj{};
        for (int b = 0; b < filledPat.size() + 1; b++) {
            shared_ptr<Words::RegularConstraints::RegOperation> current = make_shared<Words::RegularConstraints::RegOperation>(
                    Words::RegularConstraints::RegularOperator::CONCAT);

            for (int l = 0; l < b; l++) {
                current->addChild(expression->getChildren()[0]);
            }


            if (current->getChildren().empty()) {
                auto cf = encodeWord(filledPat, vector<int>{});
                disj.push_back(cf);
            } else if (current->getChildren().size() == 1) {
                auto cf = doEncode(filledPat, current->getChildren()[0]);
                disj.push_back(cf);
            } else {
                auto cf = doEncode(filledPat, current);
                disj.push_back(cf);
            }


        }
        if (disj.empty()) {
            return ffalse;
        } else if (disj.size() == 1) {
            return disj[0];
        } else {
            return PLFormula::lor(disj);
        }

    }

    PLFormula
    InductiveEncoder::encodeNone(vector<FilledPos> filledPat,
                                 shared_ptr<Words::RegularConstraints::RegEmpty> empty) {
        return ffalse;
    }

    PLFormula InductiveEncoder::encodeWord(vector<FilledPos> filledPat,
                                           vector<int> expressionIdx) {

        // Check length abstraction
        int lb = numTerminals(filledPat);
        if (expressionIdx.size() < lb || filledPat.size() < expressionIdx.size()) {
            // Unsat
            //cout << "\t WORD LA not satsified\n";
            return ffalse;
        }

        if (expressionIdx.empty()) {
            //cout << "Empty Expression\n";
            if (filledPat.empty()) {
                //cout << "SAT as both empty\n";
                return ftrue;
            } else {
                //cout << "Encoding pure lambda substitution\n";
                // All to lambda
                vector<PLFormula> conj{};
                for (auto fp: filledPat) {
                    if (fp.isTerminal()) {
                        // Can't be substituted to lambda
                        return ffalse;
                        int ci = fp.getTerminalIndex();
                        auto word = constantsVars->at(make_pair(ci, sigmaSize));
                        conj.push_back(PLFormula::lit(word));
                    } else {
                        pair<int, int> xij = fp.getVarIndex();
                        auto word = variableVars->at(make_pair(xij, sigmaSize));
                        conj.push_back(PLFormula::lit(word));
                    }
                }
                switch (conj.size()) {
                    case 0:
                        return ffalse;
                    case 1:
                        return conj[0];
                    default:
                        return PLFormula::land(conj);
                }
            }
        } // Empty Expression

        // Both are non-empty
        vector<PLFormula> disj{};
        for (int j = -1; j < int(expressionIdx.size()); j++) {
            vector<PLFormula> conj{};
            // Match prefix of size j+1
            for (int i = 0; i <= j; i++) {
                int k = expressionIdx[i];
                if (filledPat[i].isTerminal()) {
                    int ci = filledPat[i].getTerminalIndex();
                    auto word = constantsVars->at(make_pair(ci, k));
                    conj.push_back(PLFormula::lit(word));
                } else {
                    pair<int, int> xij = filledPat[i].getVarIndex();
                    auto word = variableVars->at(make_pair(xij, k));
                    conj.push_back(PLFormula::lit(word));
                }
            }
            // Match j+1-th position in pat to lambda and match pattern[j+2:] to expression[j:]
            if (j + 1 < filledPat.size()) {
                if (filledPat[j + 1].isTerminal()) {
                    // Unsat, cant set terminal to lambda
                    continue;
                } else {
                    pair<int, int> xij = filledPat[j + 1].getVarIndex();
                    auto word = variableVars->at(make_pair(xij, sigmaSize));
                    conj.push_back(PLFormula::lit(word));

                    // Remaining of this variable must also be set to lambda
                    int k = j + 2;
                    while (k < filledPat.size() && filledPat[k].isVariable()) {
                        if (filledPat[k].getVarIndex().first == xij.first) {
                            auto word = variableVars->at(make_pair(filledPat[k].getVarIndex(), sigmaSize));
                            conj.push_back(PLFormula::lit(word));
                        }
                        k++;
                    }


                    vector<FilledPos> suffixPattern(filledPat.begin() + k, filledPat.end());
                    vector<int> suffixExpression(expressionIdx.begin() + j + 1, expressionIdx.end());

                    PLFormula suffFormula = encodeWord(suffixPattern, suffixExpression);
                    conj.push_back(suffFormula);
                }
            }

            if (conj.size() == 1) {
                disj.push_back(conj[0]);
            } else if (conj.size() > 1) {
                disj.push_back(PLFormula::land(conj));
            }
        }

        if (disj.empty()) {
            return ftrue;
        } else if (disj.size() == 1) {
            return disj[0];
        } else {
            return PLFormula::lor(disj);
        }
    }


    /*********************************************************************
     ************************* Automaton Encoder *************************
     *********************************************************************/

    set<set<int>> AutomatonEncoder::encode() {

        auto total_start = high_resolution_clock::now();
        cout << "\n[*] Encoding ";
        constraint.toString(cout);
        cout << "\n";


        Words::Word pattern = constraint.pattern;
        shared_ptr<Words::RegularConstraints::RegNode> expr = constraint.expr;

        vector<FilledPos> filledPat = filledPattern(pattern);


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
        cout << "\t - Built filled NFA with " << Mxi.numStates() << " states and " << Mxi.numTransitions()
             << " transitions. Took " << duration.count() << "ms\n";



        LengthAbstraction::UNFALengthAbstractionBuilder builder(M);

        LengthAbstraction::ArithmeticProgressions rAbs = builder.forState(M.getInitialState());


        satewiseLengthAbstraction[M.getInitialState()] = rAbs;


        bool lenghtOk = false;
        for (int l = numTerminals(filledPat); l <= filledPat.size(); l++) {
            if (rAbs.contains(l)) {
                lenghtOk = true;
                break;
            }
        }
        if (!lenghtOk) {
            // Can't be satisfied
            cout << "\t - Length abstraction not satisfied\n";
            Glucose::Var v = solver.newVar();
            return set<set<int>>{set<int>{v}, set<int>{-v}};
        }


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



        /**
        set<int> visited{M.getInitialState()};
        list<int> queue;
        for (auto t: delta[M.getInitialState()]) {
            queue.push_back(t.second);
        }
        while (!queue.empty()) {
            int q = queue.front();
            queue.pop_front();
            visited.insert(q);
            LengthAbstraction::ArithmeticProgressions qrabs = builder.forState(q);
            satewiseLengthAbstraction[q] = qrabs;
            for (auto t: delta[q]) {
                if (visited.count(t.second) == 0) {
                    queue.push_back(t.second);
                }
            }
        }*/

        start = high_resolution_clock::now();
        omp_set_num_threads(8);
        for (int q=1; q<M.numStates(); q++) {
            satewiseLengthAbstraction[q] = builder.forState(q);
        }





        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        cout << "\t - Built length abstraction for each state. Took " << duration.count() << "ms\n";


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
        for (auto lit: initialConj.getSubformulae()) {
            cnf.insert(set<int>{lit.getLiteral()});
        }
        // Final States, is a disjunction of literals
        PLFormula finalDisj = encodeFinal(Mxi, filledPat);
        // Add all literals as single clause to cnf
        set<int> finalClause{};
        for (auto lit: finalDisj.getSubformulae()) {
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
            for (auto lit: disj.getSubformulae()) {
                clause.insert(lit.getLiteral());
            }
            cnf.insert(clause);
        }

        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        cout << "\t - Created [Transition] constraint. Took " << duration.count() << "ms\n";

        start = high_resolution_clock::now();
        // Predecessor constraint


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
        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        cout << "\t - Created [Predecessor] constraint (depth: " << predecessor.depth() << ", size: "
             << predecessor.size() << "). Took " << duration.count() << "ms\n";

        start = high_resolution_clock::now();
        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        set<set<int>> predecessorCnf = tseytin_cnf(predecessor, solver);
        for (const auto &clause: predecessorCnf) {
            cnf.insert(clause);
        }
        cout << "\t - Created CNF. Took " << duration.count() << "ms\n";



        /* OLD ITERATIVE APPROACH
        PLFormula predecessor = encodePredecessor(Mxi, filledPat);
        set<set<int>> predecessorCnf = tseytin_cnf(predecessor, solver);
        for (const auto &clause: predecessorCnf) {
            cnf.insert(clause);
        }*/



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

    PLFormula AutomatonEncoder::encodeFinal(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat) {
        vector<PLFormula> disj;
        int n = filledPat.size();
        for (int q: Mxi.getFinalStates()) {
            auto p = make_pair(q, n);
            disj.push_back(PLFormula::lit(stateVars[p]));
        }
        return PLFormula::lor(disj);
    }

    PLFormula AutomatonEncoder::encodeTransition(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat) {
        vector<PLFormula> disj;

        for (auto &trans: Mxi.getDelta()) {
            for (auto &target: trans.second) {
                for (int i = 0; i < filledPat.size(); i++) {


                    bool lengthOk = false;
                    for (int lb = numTerminals(filledPat, i); lb <= filledPat.size()-i; lb++) {
                        if (satewiseLengthAbstraction[trans.first].contains(lb - i)) {
                            lengthOk = true;
                            break;
                        }
                    }
                    if (!lengthOk) {
                        continue;
                    }

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


    PLFormula AutomatonEncoder::encodePredNew(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat, int q, int i,
                                              set<pair<int, int>> &visited,
                                              map<int, set<pair<Words::Terminal *, int>>> &pred) {


        visited.insert(make_pair(q, i));
        vector<PLFormula> f{};

        int currentPos = i - 1;
        // TODO: use ptr to avoid copying
        set<pair<Words::Terminal *, int>> preds{};
        set<pair<int, int>> unreachable{};
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
        while (currentPos - 1 >= 0 && filledPat[currentPos - 1].isTerminal()) {
            set<pair<Words::Terminal *, int>> predPreds{};
            for (auto p: preds) {
                for (auto pp: pred[p.second]) {
                    if (!pp.first->isEpsilon() &&
                        tIndices->at(pp.first) == filledPat.at(currentPos - 1).getTerminalIndex()) {
                        predPreds.insert(pp);

                    }
                }
                visited.insert(make_pair(p.second, currentPos));
            }
            preds = predPreds;
            currentPos--;
        }


        // If preds is empty, we now that Sqi has no valid predecessor and we'll encode -Sqi.
        vector<PLFormula> disj;
        // For all reachable states q' in succ, encode that q is the predecessor after reading i symbols
        auto delta = Mxi.getDelta();
        vector<PLFormula> conj;
        // Predecessor Sq^i
        int succVar = -stateVars[make_pair(q, i)];
        disj.push_back(PLFormula::lit(succVar));

        bool lengthOk = false;
        for (int lb = numTerminals(filledPat, i); lb <= filledPat.size()-i; lb++) {
            if (satewiseLengthAbstraction[q].contains(lb)) {
                lengthOk = true;
                break;
            }
        }


        if (lengthOk) {
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


        vector<PLFormula> disj;
        for (int i = 1; i <= filledPat.size(); i++) {
            for (int q = 0; q < Mxi.numStates(); q++) {


                bool lengthOk = false;
                vector<FilledPos> suff(filledPat.begin(), filledPat.end());
                for (int lb = numTerminals(suff); lb <= suff.size(); lb++) {
                    if (satewiseLengthAbstraction[q].contains(lb)) {
                        lengthOk = true;
                        break;
                    } else {
                        cout << lb << " not in rabs for " << q << "\n";
                    }
                }
                if (!lengthOk) {
                    continue;
                }


                vector<PLFormula> pred_q_disj; // S_q^i -> (\/ {(S_pr^i /\ w)})
                PLFormula sqi = PLFormula::lit(-stateVars[make_pair(q, i)]); // S_q^i

                // Only encode q that can be reached in Mxi with i transitions
                assert(reachable[0]->count(0) == 1);


                pred_q_disj.push_back(sqi);
                if (pred.count(q) == 1 && reachable[i]->count(q) == 1) {
                    // Has predecessor(s)
                    for (auto q_pred: pred[q]) {

                        PLFormula sqip = PLFormula::lit(stateVars[make_pair(q_pred.second, i - 1)]); // S_q'^i


                        vector<PLFormula> conj;
                        int word;
                        int k;
                        if (q_pred.first->isEpsilon()) {
                            k = sigmaSize;
                        } else {
                            k = tIndices->at(q_pred.first);
                        }
                        if (filledPat[i - 1].isTerminal()) {
                            int ci = filledPat[i - 1].getTerminalIndex();
                            if (ci != k) {
                                // UNSAT!
                                continue;
                            }
                            word = constantsVars->at(make_pair(ci, k));
                        } else {
                            pair<int, int> xij = filledPat[i - 1].getVarIndex();
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

    Automaton::NFA AutomatonEncoder::filledAutomaton(Automaton::NFA &nfa) {
        Automaton::NFA Mxi(nfa.numStates(), nfa.getDelta(), nfa.getInitialState(), nfa.getFinalStates());
        for (int q = 0; q < Mxi.numStates(); q++) {
            Mxi.add_transition(q, ctx.getEpsilon(), q);
        }
        return Mxi;
    }

}