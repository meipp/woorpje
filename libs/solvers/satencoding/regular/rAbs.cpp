
#include <algorithm>
#include <unordered_map>
#include "regencoding.h"
#include "commons.h"
#include <chrono>
using namespace std::chrono;


using namespace Words::RegularConstraints;
using namespace std;


namespace RegularEncoding {
    namespace LengthAbstraction {

        unordered_map<string, ArithmeticProgressions> cache{};

        namespace {
            ArithmeticProgressions& concat(ArithmeticProgressions& lhs, ArithmeticProgressions& rhs) {
                ArithmeticProgressions result;
                for (pair<int, int> ab1: lhs.getProgressions()) {
                    int a1 = ab1.first;
                    int b1 = ab1.second;
                    for (pair<int, int> ab2: rhs.getProgressions()) {
                        int a2 = ab2.first;
                        int b2 = ab2.second;

                        int d = __gcd(b1, b2);
                        int b = max(b1, b2);
                        int m = 0;

                        while (d*m < pow(b, 2) && d != 0) {
                            m++;
                        }
                        int nu = d*m;

                        ArithmeticProgressions P;
                        ArithmeticProgressions Q;
                        P.add(make_pair(a1+a2+nu, d));


                        int k1 = 0, k2 = 0;
                        while (b1*k1 + b2*k2 < pow(b, 2)) {
                            while (b1*k1 + b2*k2 <= pow(b, 2)) {
                                Q.add(make_pair(a1+a2+b1*k1+b2*k2, 0));
                                k2++;
                                if (b2*k2 == 0) {
                                    // Already found all k2
                                    break;
                                }
                            }
                            k2 = 0;
                            k1++;
                            if (b1*k1 == 0) {
                                // Already found all k1
                                break;
                            }
                        }

                        Q.mergeOther(P);
                        result.mergeOther(Q);
                    }
                }
            }

        }

        ArithmeticProgressions fromExpression(RegNode& expression) {

            stringstream ss;
            expression.toString(ss);
            string restr = ss.str();
            if (cache.count(restr) > 0) {
                return cache[restr];
            }
            try {
                RegWord& word = dynamic_cast<Words::RegularConstraints::RegWord&>(expression);
                // Single word w, return {(|w|, 0)}
                if (!word.word.noVariableWord()) {
                    throw new logic_error("Regular expression can't have variables");
                }
                ArithmeticProgressions ap;
                ap.add(make_pair(word.word.characters(), 0));
                cache[restr] = ap;

                return ap;
            } catch (std::bad_cast&) {}

            try {
                RegOperation& opr = dynamic_cast<Words::RegularConstraints::RegOperation&>(expression);
                switch (opr.getOperator()) {
                    case RegularOperator::CONCAT: {
                        // Concatenation
                        RegNode &lhs = *opr.getChildren()[0];
                        RegNode &rhs = *opr.getChildren()[1];
                        auto left = fromExpression(lhs);
                        auto right = fromExpression(rhs);
                        left = concat(left, right);
                        for (int i = 2; i < opr.getChildren().size(); i++) {
                            ArithmeticProgressions right = fromExpression(*opr.getChildren()[i]);
                            left.mergeOther(right);
                        }
                        cache[restr] = right;
                        return right;
                        break;
                    }
                    case RegularOperator::STAR: {
                        ArithmeticProgressions sub = fromExpression(*opr.getChildren()[0]);
                        if (sub.getProgressions().size() == 1){

                        } else {
                            vector<pair<int, int>> asvec(sub.getProgressions().begin(), sub.getProgressions().end());
                            auto ps = commons::powerset(asvec);
                            for (auto subset: ps) {
                                // TODO
                            }
                        }
                    }
                    case RegularOperator::UNION: {
                        // Union
                        ArithmeticProgressions first = fromExpression(*opr.getChildren()[0]);
                        for (int i = 1; i < opr.getChildren().size(); i++) {
                            ArithmeticProgressions next = fromExpression(*opr.getChildren()[i]);
                            first.mergeOther(next);
                        }
                        cache[restr] = first;

                        return first;
                    }
                }
            } catch (std::bad_cast&) {}

            try {
                RegEmpty& emps = dynamic_cast<Words::RegularConstraints::RegEmpty&>(expression);
            } catch (std::bad_cast&) {}

        };

    }
}