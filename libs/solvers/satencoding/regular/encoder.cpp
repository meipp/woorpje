#include <iostream>
#include <typeinfo>
#include <words/exceptions.hpp>
#include "regencoding.h"


using namespace std;
using namespace RegularEncoding::PropositionalLogic;

namespace RegularEncoding {

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

    set<set<int>> InductiveEncoder::encode() {

        cout << "\n [*] Encoding ";
        constraint.toString(cout);
        cout << "\n";

        Words::Word pattern = constraint.pattern;
        std::shared_ptr<Words::RegularConstraints::RegNode> expr = constraint.expr;

        vector<FilledPos> filledPat = filledPattern(pattern);

        PropositionalLogic::PLFormula f = doEncode(filledPat, expr);


        set<set<int>> cnf = PropositionalLogic::tseytin_cnf(f, solver);

        return cnf;
    }

    PropositionalLogic::PLFormula
    InductiveEncoder::doEncode(std::vector<FilledPos> filledPat, std::shared_ptr<Words::RegularConstraints::RegNode> expression) {
        std::shared_ptr<Words::RegularConstraints::RegWord> word = dynamic_pointer_cast<Words::RegularConstraints::RegWord>(
                expression);
        std::shared_ptr<Words::RegularConstraints::RegOperation> opr = dynamic_pointer_cast<Words::RegularConstraints::RegOperation>(
                expression);
        std::shared_ptr<Words::RegularConstraints::RegEmpty> emps = dynamic_pointer_cast<Words::RegularConstraints::RegEmpty>(
                expression);
        if (word != nullptr) {

            vector<int> expressionIdx{};
            for (auto s: word->word) {
                if (s->isTerminal()) {
                    expressionIdx.push_back(tIndices->at(s->getTerminal()));
                } else {
                    throw new Words::WordException("Only constant regular expressions allowed");
                }
            }
            return encodeWord(filledPat, expressionIdx);

        } else if (opr != nullptr) {
            switch (opr->getOperator()) {
                case Words::RegularConstraints::RegularOperator::CONCAT:
                    //return encodeConcat(filledPat, opr);
                    break;
                case Words::RegularConstraints::RegularOperator::UNION:
                    return encodeUnion(filledPat, opr);
                case Words::RegularConstraints::RegularOperator::STAR:
                    //return encodeStar(filledPat, opr);
                    break;
            }
        } else if (emps != nullptr) {

        } else {
            throw new Words::WordException("Invalid type");
        }

        return PropositionalLogic::PLFormula::lit(0);
    }

    PropositionalLogic::PLFormula
    InductiveEncoder::encodeUnion(std::vector<FilledPos> filledPat, std::shared_ptr<Words::RegularConstraints::RegOperation> expression) {
        vector<PLFormula> disj{};
        for (auto c: expression->getChildren()) {
            PLFormula f = doEncode(filledPat, c);
            disj.push_back(f);
        }
        return PLFormula::lor(disj);
    }

    PropositionalLogic::PLFormula InductiveEncoder::encodeWord(vector<FilledPos> filledPat,
                                                               vector<int> expressionIdx) {

        if (filledPat.size() < expressionIdx.size()) {
            // Unsat
            cout << "UNSAT due to |pat| < |expr|\n";
            return ffalse;
        }

        if (expressionIdx.empty()) {
            cout << "Empty Expression\n";
            if (filledPat.empty()) {
                cout << "SAT as both empty\n";
                return ftrue;
            } else {
                cout << "Encoding pure lambda substitution\n";
                // All to lambda
                vector<PLFormula> conj{};
                for (auto fp: filledPat) {
                    if (fp.isTerminal()) {
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
                    conj = vector<PLFormula>{ffalse};
                } else {
                    pair<int, int> xij = filledPat[j + 1].getVarIndex();
                    auto word = variableVars->at(make_pair(xij, sigmaSize));
                    conj.push_back(PLFormula::lit(word));

                    vector<FilledPos> suffixPattern(filledPat.begin() + j + 2, filledPat.end());
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


}