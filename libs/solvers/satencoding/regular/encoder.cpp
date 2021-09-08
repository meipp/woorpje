#include <iostream>
#include <typeinfo>
#include <words/exceptions.hpp>
#include "regencoding.h"


using namespace std;

namespace RegularEncoding {


    set<set<int>> InductiveEncoder::encode() {

        Words::Word pattern = constraint.pattern;
        std::shared_ptr<Words::RegularConstraints::RegNode> expr = constraint.expr;
        PropositionalLogic::PLFormula f = doEncode(pattern, expr);
        std::cout << "Hallo\n";

        return set<set<int>>{};
        //return PropositionalLogic::tseytin_cnf(f);
    }

    PropositionalLogic::PLFormula
    InductiveEncoder::doEncode(Words::Word pat, std::shared_ptr<Words::RegularConstraints::RegNode> expression) {
        std::shared_ptr<Words::RegularConstraints::RegWord> word = dynamic_pointer_cast<Words::RegularConstraints::RegWord>(expression);
        std::shared_ptr<Words::RegularConstraints::RegOperation> opr = dynamic_pointer_cast<Words::RegularConstraints::RegOperation>(expression);
        std::shared_ptr<Words::RegularConstraints::RegEmpty> emps = dynamic_pointer_cast<Words::RegularConstraints::RegEmpty>(expression);
        if (word != nullptr) {
            return encodeWord(pat, word);
        } else if (opr != nullptr){

        } else if (emps != nullptr) {

        } else {
            throw new Words::WordException("Invalid type");
        }

        return PropositionalLogic::PLFormula::lit(0);
    }

    PropositionalLogic::PLFormula InductiveEncoder::encodeWord(Words::Word pat,
                                                               std::shared_ptr<Words::RegularConstraints::RegWord> expression) {


    }


}