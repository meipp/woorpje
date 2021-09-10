#include <iostream>
#include <words/exceptions.hpp>
#include "regencoding.h"
#include <chrono>
using namespace std::chrono;
#include "boost/container_hash/hash.hpp"

using namespace std;
using namespace RegularEncoding::PropositionalLogic;

const int UNROLLBOUND = 8;


namespace RegularEncoding {

    namespace {

        int numTerminals(vector<FilledPos>& pat) {
            int ts = 0;
            for(auto fp: pat) {
                if (fp.isTerminal()) {
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

    set<set<int>> InductiveEncoder::encode() {

        cout << "\n[*] Encoding ";
        constraint.toString(cout);
        cout << "\n";

        Words::Word pattern = constraint.pattern;
        std::shared_ptr<Words::RegularConstraints::RegNode> expr = constraint.expr;

        vector<FilledPos> filledPat = filledPattern(pattern);

        auto start = high_resolution_clock::now();
        PropositionalLogic::PLFormula f = doEncode(filledPat, expr);

        cout << "\t - Built formula with depth " << f.depth() << ", creating CNF\n";

        set<set<int>> cnf = PropositionalLogic::tseytin_cnf(f, solver);
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        cout << "\t - CNF done, "<< cnf.size() << " clauses\n";
        cout << "[*] Encoding done. Took " << duration.count() << "ms" << endl;
        return cnf;
    }

    PropositionalLogic::PLFormula
    InductiveEncoder::doEncode(std::vector<FilledPos> filledPat,
                               std::shared_ptr<Words::RegularConstraints::RegNode> expression) {

        /*
        stringstream esstr;
        expression->toString(esstr);
        vector<size_t> hashes{boost::hash_value(esstr.str())};
        for(auto c: filledPat) {
            if (c.isTerminal()) {
                hashes.push_back(boost::hash_value(c.getTerminalIndex()));
            } else {
                hashes.push_back(boost::hash_value(c.getVarIndex()));
            }
        }
        size_t consthash = boost::hash_range(hashes.begin(), hashes.end());


        if (ccache.count(consthash) == 1) {
            return  *ccache.at(consthash);
        }*/

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
            PLFormula result = encodeWord(filledPat, expressionIdx);
            //ccache[consthash] = &result;
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

        } else {
            throw new Words::WordException("Invalid type");
        }

        return PropositionalLogic::PLFormula::lit(0);
    }

    PropositionalLogic::PLFormula
    InductiveEncoder::encodeUnion(std::vector<FilledPos> filledPat,
                                  std::shared_ptr<Words::RegularConstraints::RegOperation> expression) {
        vector<PLFormula> disj{};
        LengthAbstraction::ArithmeticProgressions la = LengthAbstraction::fromExpression(*expression);
        for (auto c: expression->getChildren()) {
            // TODO: Check length abstraction
            bool lengthOk = false;
            // < or <= ?? Also, for other cases?
            for (int i = numTerminals(filledPat); i<=filledPat.size(); i++) {
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
    std::shared_ptr<Words::RegularConstraints::RegOperation>
    makeNodeBinary(std::shared_ptr<Words::RegularConstraints::RegOperation> concat) {
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

    PropositionalLogic::PLFormula
    InductiveEncoder::encodeConcat(std::vector<FilledPos> filledPat,
                                   std::shared_ptr<Words::RegularConstraints::RegOperation> expression) {

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

            for(int i = numTerminals(prefix); i <= prefix.size(); i++ ) {
                if (laL.contains(i)) {
                    lengthOk = true;
                    break;
                }
            }

            if (!lengthOk){
                continue;
            }
            lengthOk = false;
            for(int i = numTerminals(suffix); i <= suffix.size(); i++ ) {
                if (laR.contains(i)) {
                    lengthOk = true;
                    break;
                }
            }


            if (!lengthOk) {
                //At least one does not satisfy length abstraction
                //cout << "\t CONCAT LA not satsified\n";
                cout << "\tLA for suffix failed\n";
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

    PropositionalLogic::PLFormula
    InductiveEncoder::encodeStar(std::vector<FilledPos> filledPat,
               std::shared_ptr<Words::RegularConstraints::RegOperation> expression) {

        LengthAbstraction::ArithmeticProgressions la = LengthAbstraction::fromExpression(*expression);

        bool lengthOk = false;
        // < or <= ?? Also, for other cases?
        for (int i = numTerminals(filledPat); i<=filledPat.size(); i++) {
            if (la.contains(i)) {
                lengthOk = true;
                break;
            }
        }

        if (!lengthOk) {
            return ffalse;
        }

        vector<PLFormula> disj{};
        for (int b = 0; b < UNROLLBOUND; b++) {
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
        } else if(disj.size() == 1) {
            return disj[0];
        } else {
            return PLFormula::lor(disj);
        }

    }

    PropositionalLogic::PLFormula
    InductiveEncoder::encodeNone(std::vector<FilledPos> filledPat,
               std::shared_ptr<Words::RegularConstraints::RegEmpty> empty) {
        return ffalse;
    }

    PropositionalLogic::PLFormula InductiveEncoder::encodeWord(vector<FilledPos> filledPat,
                                                               vector<int> expressionIdx) {

        // Check length abstraction
        int lb = numTerminals(filledPat);
        if (expressionIdx.size() < lb || filledPat.size() < expressionIdx.size() ) {
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
                    conj = vector<PLFormula>{ffalse};
                } else {
                    pair<int, int> xij = filledPat[j + 1].getVarIndex();
                    auto word = variableVars->at(make_pair(xij, sigmaSize));
                    conj.push_back(PLFormula::lit(word));

                    // Remaining of this variable must also be set to lambda
                    int k=j+2;
                    while(k < filledPat.size() && filledPat[k].isVariable()) {
                        if (filledPat[k].getVarIndex().first == xij.first){
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


}