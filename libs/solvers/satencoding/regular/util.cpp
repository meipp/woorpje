#include <set>
#include <unordered_set>
#include <vector>
#include "regencoding.h"


using namespace std;
using namespace Words::RegularConstraints;


namespace RegularEncoding {
    Words::RegularConstraints::RegConstraint stripPrefix(Words::RegularConstraints::RegConstraint original) {

        string prefix;
        vector<Words::IEntry*> suffixVec;
        bool inPrefix = true;
        for (auto w: original.pattern) {
            if (w->isTerminal() && inPrefix) {
                prefix.push_back(w->getTerminal()->getChar());
            } else {
                inPrefix = false;
                suffixVec.push_back(w);
            }
        }
        shared_ptr<RegNode> stripped = original.expr;
        for (char pc: prefix) {
            stripped = stripped->derivative(string(1, pc));
        }

        Words::Word suffix(move(suffixVec));
        return {suffix, stripped};

    }

    Words::RegularConstraints::RegConstraint stripSuffix(Words::RegularConstraints::RegConstraint original) {
        // Revers
        auto reversedExpr = original.expr->reverse();
        vector<Words::IEntry*> reversedVec;
        for (auto w: original.pattern) {
            reversedVec.push_back(w);
        }
        std::reverse(reversedVec.begin(), reversedVec.end());
        Words::Word reversedWord(move(reversedVec));

        RegConstraint reversedConstr(reversedWord, reversedExpr);
        Words::RegularConstraints::RegConstraint strippedRev = stripPrefix(reversedConstr);

        // Revers
        auto rereversedExpr = strippedRev.expr->reverse();
        vector<Words::IEntry*> rereversedVec;
        for (auto w: strippedRev.pattern) {
            rereversedVec.push_back(w);
        }
        std::reverse(rereversedVec.begin(), rereversedVec.end());
        Words::Word rereversedWord(move(rereversedVec));
        return {rereversedWord, rereversedExpr};

    }



}