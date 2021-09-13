#include <set>
#include <unordered_set>
#include <vector>
#include "regencoding.h"


using namespace std;
using namespace Words::RegularConstraints;


template<>
struct std::hash<std::vector<std::shared_ptr<Words::RegularConstraints::RegNode>>> {
    std::size_t operator()(const vector<shared_ptr<RegNode>> &k) const {

        return ((hash<string>()(k[0]->toString())
                 ^ (hash<string>()(k[1]->toString()) << 1)));
    }
};

template<>
struct std::equal_to<std::vector<std::shared_ptr<Words::RegularConstraints::RegNode>>> {
    bool operator()(const vector<shared_ptr<RegNode>> &k, const vector<shared_ptr<RegNode>> &k2) const {


        return hash<string>()(k[0]->toString()) == hash<string>()(k2[0]->toString()) &&
               hash<string>()(k[1]->toString()) == hash<string>()(k2[1]->toString());

    }
};

namespace RegularEncoding {

    std::set<std::vector<std::shared_ptr<Words::RegularConstraints::RegNode>>>
    partitioning(const std::shared_ptr<Words::RegularConstraints::RegNode> &expression, Words::Context ctx) {

        unordered_set<vector<shared_ptr<RegNode>>,
                std::hash<std::vector<std::shared_ptr<Words::RegularConstraints::RegNode>>>
        > partition{};

        shared_ptr<RegWord> word = dynamic_pointer_cast<RegWord>(expression);
        shared_ptr<RegOperation> opr = dynamic_pointer_cast<RegOperation>(expression);

        if (word != nullptr) {
            for (int i = 0; i <= word->word.characters(); i++) {
                Words::Word prefWord;
                Words::Word suffWord;
                unique_ptr<Words::WordBuilder> prefWordBuilder = ctx.makeWordBuilder(prefWord);
                unique_ptr<Words::WordBuilder> suffWordBuilder = ctx.makeWordBuilder(suffWord);
                int j = 0;
                for (auto w: word->word) {
                    if (j < i) {
                        *prefWordBuilder << w->getTerminal()->getChar();
                    }
                    j++;
                }
                prefWordBuilder->flush();
                int jj = 0;
                for (auto w: word->word) {

                    if (jj >= i && jj < word->word.characters()) {
                        *suffWordBuilder << w->getTerminal()->getChar();
                    }
                    jj++;
                }
                suffWordBuilder->flush();

                auto parts = vector<shared_ptr<RegNode>>{make_shared<RegWord>(RegWord(prefWord)),
                                                         make_shared<RegWord>(RegWord(suffWord))};

                partition.insert(parts);

            }

        } else if (opr != nullptr) {
            switch (opr->getOperator()) {
                case RegularOperator::CONCAT: {
                    if (opr->getChildren().size() > 2) {
                        return partitioning(makeNodeBinary(opr), ctx);
                    }
                    auto lhs = partitioning(opr->getChildren()[0], ctx);
                    auto rhs = partitioning(opr->getChildren()[1], ctx);
                    for (auto &lhsp: lhs) {
                        for (auto &rhsp: rhs) {


                            auto first = vector<shared_ptr<RegNode>>{
                                    make_shared<RegWord>(RegWord(Words::Word())),
                                    make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT,
                                                                           vector<shared_ptr<RegNode>>{lhsp[0], lhsp[1],
                                                                                                       rhsp[0],
                                                                                                       rhsp[1]}))
                            };
                            partition.insert(first);


                            auto scnd = vector<shared_ptr<RegNode>>{
                                    lhsp[0],
                                    make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT,
                                                                           vector<shared_ptr<RegNode>>{lhsp[1], rhsp[0],
                                                                                                       rhsp[1]}))
                            };

                            auto third = vector<shared_ptr<RegNode>>{
                                    make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT,
                                                                           vector<shared_ptr<RegNode>>{lhsp[0],
                                                                                                       lhsp[1]})),
                                    make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT,
                                                                           vector<shared_ptr<RegNode>>{rhsp[0],
                                                                                                       rhsp[1]}))
                            };
                            auto forth = vector<shared_ptr<RegNode>>{
                                    make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT,
                                                                           vector<shared_ptr<RegNode>>{lhsp[0], lhsp[1],
                                                                                                       rhsp[0]})),
                                    rhsp[1]
                            };

                            auto fifth = vector<shared_ptr<RegNode>>{
                                    make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT,
                                                                           vector<shared_ptr<RegNode>>{lhsp[0], lhsp[1],
                                                                                                       rhsp[0],
                                                                                                       rhsp[1]})),
                                    make_shared<RegWord>(RegWord(Words::Word())),
                            };

                            partition.insert(scnd);
                            partition.insert(third);
                            partition.insert(forth);
                            partition.insert(fifth);

                        }
                    }
                    break;
                }
                case RegularOperator::STAR: {
                    auto part = partitioning(opr->getChildren()[0], ctx);
                    for (auto p: part) {
                        partition.insert(vector<shared_ptr<RegNode>>{p[0],
                                                                     make_shared<RegOperation>(RegularOperator::CONCAT,
                                                                                               vector<shared_ptr<RegNode>>{
                                                                                                       p[1], opr})});
                        partition.insert(vector<shared_ptr<RegNode>>{make_shared<RegOperation>(RegularOperator::CONCAT,
                                                                                               vector<shared_ptr<RegNode>>{
                                                                                                       opr, p[0]}),
                                                                     p[1]});
                        partition.insert(
                                vector<shared_ptr<RegNode>>{opr, make_shared<RegWord>(RegWord(Words::Word()))});
                        partition.insert(
                                vector<shared_ptr<RegNode>>{make_shared<RegWord>(RegWord(Words::Word())), opr});
                    }
                    break;
                }
                case RegularOperator::UNION: {
                    for (auto &child: opr->getChildren()) {
                        auto p = partitioning(child, ctx);
                        partition.insert(p.begin(), p.end());
                    }
                    break;
                }
            }
        }


        set<vector<shared_ptr<RegNode>>> partitionSet;


        return {
                partition.

                        begin(), partition

                        .

                                end()

        };


    }
}