#include "words/regconstraints.hpp"

namespace Words::RegularConstraints {

Words::RegularConstraints::RegConstraint stripPrefix(Words::RegularConstraints::RegConstraint original) {
    std::string prefix;
    std::vector<Words::IEntry *> suffixVec;
    bool inPrefix = true;
    for (auto w : original.pattern) {
        if (w->isTerminal() && inPrefix) {
            prefix.push_back(w->getTerminal()->getChar());
        } else {
            inPrefix = false;
            suffixVec.push_back(w);
        }
    }
    std::shared_ptr<RegNode> stripped = original.expr;
    for (char pc : prefix) {
        stripped = stripped->derivative(std::string(1, pc));
    }

    Words::Word suffix(move(suffixVec));
    return {suffix, stripped};
}

Words::RegularConstraints::RegConstraint stripSuffix(Words::RegularConstraints::RegConstraint original) {
    // Revers
    auto reversedExpr = original.expr->reverse();
    std::vector<Words::IEntry *> reversedVec;
    for (auto w : original.pattern) {
        reversedVec.push_back(w);
    }
    std::reverse(reversedVec.begin(), reversedVec.end());
    Words::Word reversedWord(move(reversedVec));

    RegConstraint reversedConstr(reversedWord, reversedExpr);
    Words::RegularConstraints::RegConstraint strippedRev = stripPrefix(reversedConstr);

    // Revers
    auto rereversedExpr = strippedRev.expr->reverse();
    std::vector<Words::IEntry *> rereversedVec;
    for (auto w : strippedRev.pattern) {
        rereversedVec.push_back(w);
    }
    std::reverse(rereversedVec.begin(), rereversedVec.end());
    Words::Word rereversedWord(move(rereversedVec));
    return {rereversedWord, rereversedExpr};
}
}