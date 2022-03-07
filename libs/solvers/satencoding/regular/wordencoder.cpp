#include "encoding.h"
#include "regencoding.h"
#include <set>
#include <words/exceptions.hpp>
#include <chrono>


using namespace std;
using namespace chrono;
using namespace RegularEncoding::PropositionalLogic;

namespace RegularEncoding {
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
    for (const auto &cl : cnf) {
        litTotal += int(cl.size());
    }

    auto stopEncoding = chrono::high_resolution_clock::now();
    auto durationEncoding = duration_cast<milliseconds>(stopEncoding - startEncoding);
    cout << "\t - CNF done, " << cnf.size() << " clauses and " << litTotal << " literals in total\n";
    cout << "[*] Encoding done. Took " << durationEncoding.count() << "ms" << endl;
    return cnf;
}

PLFormula InductiveEncoder::doEncode(const vector<FilledPos> &filledPat, const shared_ptr<Words::RegularConstraints::RegNode> &expression) {
    shared_ptr<Words::RegularConstraints::RegWord> word = dynamic_pointer_cast<Words::RegularConstraints::RegWord>(expression);
    shared_ptr<Words::RegularConstraints::RegOperation> opr = dynamic_pointer_cast<Words::RegularConstraints::RegOperation>(expression);
    shared_ptr<Words::RegularConstraints::RegEmpty> emps = dynamic_pointer_cast<Words::RegularConstraints::RegEmpty>(expression);

    bool isConst = true;
    for (auto fp : filledPat) {
        if (fp.isVariable()) {
            isConst = false;
            break;
        }
    }
    if (isConst) {
        auto nfa = Automaton::regexToNfa(*expression, ctx);
        Words::Word constW;
        unique_ptr<Words::WordBuilder> wb = ctx.makeWordBuilder(constW);
        for (auto fp : filledPat) {
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
        for (auto s : word->word) {
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

PLFormula InductiveEncoder::encodeUnion(vector<FilledPos> filledPat, const shared_ptr<Words::RegularConstraints::RegOperation> &expression) {
    vector<PLFormula> disj{};
    LengthAbstraction::ArithmeticProgressions la = LengthAbstraction::fromExpression(*expression);

    for (const auto &c : expression->getChildren()) {
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
shared_ptr<Words::RegularConstraints::RegOperation> makeNodeBinary(shared_ptr<Words::RegularConstraints::RegOperation> concat) {
    if (concat->getChildren().size() <= 2) {
        return concat;
    }

    // New root
    shared_ptr<Words::RegularConstraints::RegOperation> root = make_shared<Words::RegularConstraints::RegOperation>(Words::RegularConstraints::RegularOperator::CONCAT);

    auto lhs = concat->getChildren()[0];
    shared_ptr<Words::RegularConstraints::RegOperation> rhs = make_shared<Words::RegularConstraints::RegOperation>(Words::RegularConstraints::RegularOperator::CONCAT);
    for (int i = 1; i < concat->getChildren().size(); i++) {
        rhs->addChild(concat->getChildren()[i]);
    }

    // Add new children
    root->addChild(lhs);
    root->addChild(rhs);

    return root;
}

PLFormula InductiveEncoder::encodeConcat(vector<FilledPos> filledPat, const shared_ptr<Words::RegularConstraints::RegOperation> &expression) {
    if (expression->getChildren().size() > 2) {
        auto bin = makeNodeBinary(expression);
        return encodeConcat(filledPat, bin);
    }

    vector<PLFormula> disj{};
    auto L = expression->getChildren()[0];
    auto R = expression->getChildren()[1];

    shared_ptr<Words::RegularConstraints::RegWord> rAsWord = dynamic_pointer_cast<Words::RegularConstraints::RegWord>(R);
    shared_ptr<Words::RegularConstraints::RegWord> lAsWord = dynamic_pointer_cast<Words::RegularConstraints::RegWord>(L);
    shared_ptr<Words::RegularConstraints::RegEmpty> rAsEmpty = dynamic_pointer_cast<Words::RegularConstraints::RegEmpty>(R);
    shared_ptr<Words::RegularConstraints::RegEmpty> lAsEmpty = dynamic_pointer_cast<Words::RegularConstraints::RegEmpty>(L);

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

PLFormula InductiveEncoder::encodeStar(vector<FilledPos> filledPat, const shared_ptr<Words::RegularConstraints::RegOperation> &expression) {
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
        shared_ptr<Words::RegularConstraints::RegOperation> current = make_shared<Words::RegularConstraints::RegOperation>(Words::RegularConstraints::RegularOperator::CONCAT);

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

PLFormula InductiveEncoder::encodeNone(const vector<FilledPos> &filledPat, const shared_ptr<Words::RegularConstraints::RegEmpty> &empty) { return ffalse; }

PLFormula InductiveEncoder::encodeWord(vector<FilledPos> filledPat, vector<int> expressionIdx) {
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
            for (auto fp : filledPat) {
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
    }  // Empty Expression

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
            while (k < filledPat.size() && filledPat[k].isVariable() && filledPat[k].getVarIndex().first == xij.first) {
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
}  // namespace RegularEncoding