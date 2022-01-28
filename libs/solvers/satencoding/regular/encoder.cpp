#include <iostream>
#include <words/exceptions.hpp>
#include "regencoding.h"
#include <chrono>
#include <omp.h>


using namespace std;
using namespace RegularEncoding::PropositionalLogic;
using namespace chrono;


namespace RegularEncoding {

    namespace {

        int numTerminals(vector<FilledPos> &pat, int from = 0, int to = -1) {
            if (to < 0) {
                to = pat.size();
            }
            int ts = 0;
            for (int i = from; i < to; i++) {
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
    vector<FilledPos> Encoder::filledPattern(const Words::Word &pattern) {
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
    int skipped = 0;

    set<set<int>> InductiveEncoder::encode() {
        skipped = 0;
        auto startEncoding = chrono::high_resolution_clock::now();
        cout << "\n[*] Encoding ";
        constraint.toString(cout);
        cout << "\n";


        Words::Word pattern = constraint.pattern;
        shared_ptr<Words::RegularConstraints::RegNode> expr = constraint.expr;

        vector<FilledPos> filledPat = filledPattern(pattern);
        profiler.patternSize = filledPat.size();

        auto startRabs = high_resolution_clock::now();
        LengthAbstraction::fromExpression(*expr);
        auto stopRabs = high_resolution_clock::now();
        auto rabsduration = duration_cast<milliseconds>(stopRabs - startRabs);
        profiler.timeLengthAbstraction = rabsduration.count();

        auto startFormula = high_resolution_clock::now();
        PLFormula f = doEncode(filledPat, expr);
        auto stopFormula = high_resolution_clock::now();
        auto durationFormula = duration_cast<milliseconds>(stopFormula - startFormula);
        profiler.timeFormula = durationFormula.count();
        cout << "\t - Built formula (depth: " << f.depth() << ", size:" << f.size() << ") creating CNF\n";
        cout << "\t\t - Skipped " << skipped << " encodings due to length abstraction\n";

        profiler.skipped = skipped;

        auto startTsey = high_resolution_clock::now();
        set<set<int>> cnf = tseytin_cnf(f, solver);
        auto stopTsy = high_resolution_clock::now();
        auto durationTsey = duration_cast<milliseconds>(stopTsy - startTsey);
        profiler.timeTseytin = durationTsey.count();


        int litTotal = 0;
        for (const auto &cl: cnf) {
            litTotal += int(cl.size());
        }

        auto stopEncoding = chrono::high_resolution_clock::now();
        auto durationEncoding = duration_cast<milliseconds>(stopEncoding - startEncoding);
        cout << "\t - CNF done, " << cnf.size() << " clauses and " << litTotal << " literals in total\n";
        cout << "[*] Encoding done. Took " << durationEncoding.count() << "ms" << endl;
        return cnf;
    }

    PLFormula
    InductiveEncoder::doEncode(const vector<FilledPos> &filledPat,
                               const shared_ptr<Words::RegularConstraints::RegNode> &expression) {


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
                }
                case Words::RegularConstraints::RegularOperator::UNION: {
                    return encodeUnion(filledPat, opr);
                }
                case Words::RegularConstraints::RegularOperator::STAR: {
                    return encodeStar(filledPat, opr);
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
                                  const shared_ptr<Words::RegularConstraints::RegOperation> &expression) {
        vector<PLFormula> disj{};
        LengthAbstraction::ArithmeticProgressions la = LengthAbstraction::fromExpression(*expression);


        for (const auto &c: expression->getChildren()) {
            bool lengthOk = false;
            for (int i = numTerminals(filledPat); i <= filledPat.size(); i++) {
                if (la.contains(i)) {
                    lengthOk = true;
                    break;
                }
                skipped++;
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
                                   const shared_ptr<Words::RegularConstraints::RegOperation> &expression) {


        if (expression->getChildren().size() > 2) {
            auto bin = makeNodeBinary(expression);
            return encodeConcat(filledPat, bin);
        }


        vector<PLFormula> disj{};
        auto L = expression->getChildren()[0];
        auto R = expression->getChildren()[1];

        shared_ptr<Words::RegularConstraints::RegWord> rAsWord = dynamic_pointer_cast<Words::RegularConstraints::RegWord>(
                R);
        shared_ptr<Words::RegularConstraints::RegWord> lAsWord = dynamic_pointer_cast<Words::RegularConstraints::RegWord>(
                L);
        shared_ptr<Words::RegularConstraints::RegEmpty> rAsEmpty = dynamic_pointer_cast<Words::RegularConstraints::RegEmpty>(
                R);
        shared_ptr<Words::RegularConstraints::RegEmpty> lAsEmpty = dynamic_pointer_cast<Words::RegularConstraints::RegEmpty>(
                L);


        if (rAsEmpty != nullptr || lAsEmpty != nullptr) {
            // concat with emptyset
            return ffalse;
        }


        if (rAsWord != nullptr && rAsWord->characters() == 0) {
            // Concat with empty word
            return doEncode(filledPat, L);
        }
        if (lAsWord != nullptr && lAsWord->characters() == 0) {
            // Concat with empty word
            return doEncode(filledPat, R);
        }


        LengthAbstraction::ArithmeticProgressions laL = LengthAbstraction::fromExpression(*L);
        LengthAbstraction::ArithmeticProgressions laR = LengthAbstraction::fromExpression(*R);

        for (int i = 0; i <= int(filledPat.size()); i++) {

            vector<FilledPos> prefix(filledPat.begin(), filledPat.begin() + i);
            vector<FilledPos> suffix(filledPat.begin() + i, filledPat.end());


            // Check length abstractions
            bool lengthOk = false;

            for (int k = numTerminals(prefix); k <= prefix.size(); k++) {
                if (laL.contains(k)) {
                    lengthOk = true;
                    break;
                }
            }

            if (!lengthOk) {
                skipped++;
                continue;
            }
            lengthOk = false;
            for (int k = numTerminals(suffix); k <= suffix.size(); k++) {
                if (laR.contains(k)) {
                    lengthOk = true;
                    break;
                }
            }

            if (!lengthOk) {
                continue;
            }

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
                                 const shared_ptr<Words::RegularConstraints::RegOperation> &expression) {

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
            skipped++;
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
    InductiveEncoder::encodeNone(const vector<FilledPos> &filledPat,
                                 const shared_ptr<Words::RegularConstraints::RegEmpty> &empty) {
        return ffalse;
    }

    PLFormula InductiveEncoder::encodeWord(vector<FilledPos> filledPat,
                                           vector<int> expressionIdx) {


        // Check length abstraction
        int lb = numTerminals(filledPat);
        if (filledPat.size() < expressionIdx.size() || expressionIdx.size() < lb) {
            // Unsat
            skipped++;
            return ffalse;
        }
        if (expressionIdx.empty()) {

            if (filledPat.empty()) {

                return ftrue;
            } else {

                // All to lambda
                vector<PLFormula> conj{};
                for (auto fp: filledPat) {
                    if (fp.isTerminal()) {
                        // Can't be substituted to lambda
                        return ffalse;
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

            // Skip prefixes of size j where j+1 is terminal
            if (j + 1 < filledPat.size() && filledPat[j + 1].isTerminal()) {
                // Unsat, cant set terminal to lambda
                continue;
            }

            bool valid = true;
            vector<PLFormula> conj{};
            // Match prefix of size j+1
            for (int i = 0; i <= j; i++) {
                int k = expressionIdx[i];
                if (filledPat[i].isTerminal()) {
                    int ci = filledPat[i].getTerminalIndex();
                    if (ci != k) {
                        valid = false;
                        break;
                    }
                    auto word = constantsVars->at(make_pair(ci, k));
                    conj.push_back(PLFormula::lit(word));
                } else {
                    pair<int, int> xij = filledPat[i].getVarIndex();
                    auto word = variableVars->at(make_pair(xij, k));
                    conj.push_back(PLFormula::lit(word));
                }
            }
            if (!valid) {
                break;
            }

            // Match j+1-th position in pat to lambda and match pattern[j+2:] to expression[j:]
            if (j + 1 < filledPat.size()) {
                pair<int, int> xij = filledPat[j + 1].getVarIndex();
                auto word = variableVars->at(make_pair(xij, sigmaSize));
                conj.push_back(PLFormula::lit(word));

                // Remaining of this variable must also be set to lambda
                int k = j + 2;
                while (k < filledPat.size() && filledPat[k].isVariable() &&
                       filledPat[k].getVarIndex().first == xij.first) {

                    auto wordLambda = variableVars->at(make_pair(filledPat[k].getVarIndex(), sigmaSize));
                    conj.push_back(PLFormula::lit(wordLambda));

                    k++;
                }


                vector<FilledPos> suffixPattern(filledPat.begin() + k, filledPat.end());
                vector<int> suffixExpression(expressionIdx.begin() + j + 1, expressionIdx.end());

                PLFormula suffFormula = encodeWord(suffixPattern, suffixExpression);
                conj.push_back(suffFormula);
            }

            if (conj.size() == 1) {
                disj.push_back(conj[0]);
            } else if (conj.size() > 1) {
                disj.push_back(PLFormula::land(conj));
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


        // Waymatrix
        start = high_resolution_clock::now();
        this->waymatrix = LengthAbstraction::waymatrix(M, filledPat.size());
        stop = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<milliseconds>(stop - start);
        cout << "\t - Built waymatrix with "
             << waymatrix[0].size() << "x" << waymatrix[0].size()
             << "states and " << filledPat.size() << " exponents. Took " << duration.count() << "ms\n";


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
                for (auto &target: trans.second) {


                    vector<PLFormula> clause;

                    bool lengthOk = false;
                    for (int lb = numTerminals(filledPat, i); lb <= filledPat.size() - i; lb++) {
                        for (int fs: Mxi.getFinalStates()) {
                            if (this->waymatrix[lb][trans.first][fs]) {
                                lengthOk = true;
                                break;
                            }
                        }
                    }
                    if (!lengthOk) {
                        continue;
                    }


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

                bool lengthOk = false;
                for (int lb = numTerminals(filledPat, i); lb <= filledPat.size() - i; lb++) {
                    for (int fs: Mxi.getFinalStates()) {
                        if (lb == 0 && fs == q) {
                            lengthOk = true;
                            break;
                        }
                        if (this->waymatrix[lb][q][fs]) {
                            lengthOk = true;
                            break;
                        }
                    }
                }
                if (!lengthOk) {
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


        int currentPos = i - 1; // Position of i in the pattern

        bool lenghtOk = false;

        for (int lb = numTerminals(filledPat, 0, i); lb <= i; lb++) {


            if (this->waymatrix[lb][Mxi.getInitialState()][q]) {
                lenghtOk = true;
                break;
            }
        }if (!lenghtOk && i > 1) {
            // Seems to introduce pure literals
            int succVar = -stateVars[make_pair(q, i)];
            return PLFormula::lit(succVar);
        }

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
        Automaton::NFA Mxi(nfa.numStates(), nfa.getDelta(), nfa.getInitialState(), nfa.getFinalStates());
        for (int q = 0; q < Mxi.numStates(); q++) {
            Mxi.add_transition(q, ctx.getEpsilon(), q);
        }
        return Mxi;
    }

}