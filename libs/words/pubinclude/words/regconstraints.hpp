#include <string>
#include<vector>
#include "smtparser/ast.hpp"
#include "words/words.hpp"
#include <boost/functional/hash.hpp>


namespace Words {
    namespace RegularConstraints {
        /**
         * ABC for regular expression trees.
         */
        class RegNode {
        public:
            //virtual RegNode deepCopy();
            RegNode() {};

            virtual ~RegNode() {}

            virtual void toString(std::ostream &os) = 0;

            virtual int numStars() {return 0;};
            virtual int starHeight() {return 0;};
            virtual int depth() {return 0;};

            virtual bool isEmpty() {return false;};

            std::string toString() {
                if (str == "-") {
                    std::stringstream ss;
                    toString(ss);
                    str = ss.str();
                }
                return str;
            };

            virtual void flatten(){};

            virtual int longestLiteral(){return 0;};

            virtual int shortestLiteral(){return 0;};

            virtual std::shared_ptr<RegNode> reverse() = 0;

            virtual std::shared_ptr<RegNode> derivative(std::string d) = 0;

            virtual size_t characters() = 0;

            size_t hash() {
                boost::hash<std::string> strhash;
                return strhash(toString());
            }

            virtual bool isLeaf() { return false; }

            virtual void getAlphabet(Words::WordBuilder wb) = 0;

            virtual bool acceptsEpsilon() = 0;

            virtual long complexity(){return 0;};


        private:
            std::string str = "-";


        };



        /**
         * Current supported regular operators.
         */
        enum class RegularOperator {
            UNION, CONCAT, STAR
        };

        class RegEmpty : public RegNode {
        public:
            RegEmpty() {};

            bool acceptsEpsilon() override { return false; }

            bool isEmpty() override { return true; }

            virtual bool isLeaf() override { return true; }

            int longestLiteral() override { return 0; }

            void toString(std::ostream &os) override { os << "</>"; }

            long complexity() override { return 1; }

            void getAlphabet(Words::WordBuilder) override {}

            size_t characters() override { return 0; }

            std::shared_ptr<RegNode> derivative(std::string) override {
                return std::make_shared<RegEmpty>(RegEmpty());
            }

            std::shared_ptr<RegNode> reverse() override {
                return std::make_shared<RegEmpty>(RegEmpty());
            }

        };

        class RegWord : public RegNode {
        public:
            //RegWord(std::string word) : word(word){};
            RegWord(Words::Word word) : word(word) {};

            void toString(std::ostream &os) override {
                for (auto c: word)
                    os << c->getTerminal()->getChar();
            }

            size_t characters() override {
                return word.characters();
            }

            bool isEmpty() override {return false;}

            long complexity() override { return characters(); }

            int longestLiteral() override { return characters(); }

            int shortestLiteral() override { return characters(); }

            bool acceptsEpsilon() override { return word.characters() == 0; }

            std::shared_ptr<RegNode> derivative(std::string d) override {
                if (d.empty()) {
                    return std::make_shared<RegWord>(RegWord(word));
                }
                if (d.size() > word.characters()) {
                    return std::make_shared<RegEmpty>(RegEmpty());
                }
                std::vector<IEntry *> deriv;
                for (auto w: word) {
                    if (!d.empty()) {
                        if (w->getTerminal()->getChar() == d.front()) {
                            d.erase(0, 1);
                        } else {
                            return std::make_shared<RegEmpty>(RegEmpty());
                        }
                    } else {
                        deriv.push_back(w);
                    }
                }
                Word derivWord(std::move(deriv));
                return std::make_shared<RegWord>(RegWord(derivWord));
            }

            std::shared_ptr<RegNode> reverse() override {
                std::vector<IEntry *> revEntries;
                for (auto w: word) {
                    revEntries.push_back(w);
                }
                std::reverse(revEntries.begin(), revEntries.end());
                Word ww(std::move(revEntries));
                auto res = std::make_shared<RegWord>(RegWord(ww));
                return res;
            }

            virtual bool isLeaf() override { return true; }

            void getAlphabet(Words::WordBuilder wb) override {
                for (auto e: word) {
                    if (e->isTerminal()) {
                        wb << e->getTerminal()->getChar();
                    }
                }
            }

            Words::Word word;
        };

        /**
         * Node representing a regular operation.
         * Owns a list of children as the operands.
         */
        class RegOperation : public RegNode {
        public:
            RegOperation(RegularOperator regOperator)
                    : RegOperation(regOperator, std::vector<std::shared_ptr<RegNode>>{}) {};

            RegOperation(RegularOperator regOperator, std::vector<std::shared_ptr<RegNode>> children)
                    : op(regOperator), children(children) {};

            /**
             * Adds a child (i.e. operand) to this node.
             */
            void addChild(std::shared_ptr<RegNode> node) {
                children.push_back(node);
            };

            virtual bool isLeaf() override { return false; }

            std::shared_ptr<RegNode> reverse() override {
                switch (op) {
                    case RegularOperator::UNION: {
                        std::vector<std::shared_ptr<RegNode>> chdRev{};
                        for (auto ch: getChildren()) {
                            chdRev.push_back(ch->reverse());
                        }
                        return std::make_shared<RegOperation>(RegOperation(RegularOperator::UNION, chdRev));
                        break;
                    }
                    case RegularOperator::CONCAT: {
                        std::vector<std::shared_ptr<RegNode>> chdRev{};
                        for (auto ch: getChildren()) {
                            chdRev.push_back(ch->reverse());
                        }
                        std::reverse(chdRev.begin(), chdRev.end());
                        return std::make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT, chdRev));
                    }
                    case RegularOperator::STAR: {
                        std::vector<std::shared_ptr<RegNode>> chdRev{getChildren()[0]->reverse()};
                        return std::make_shared<RegOperation>(RegOperation(RegularOperator::STAR, chdRev));
                        break;
                    }
                }
            };

            bool isEmpty() override {
                switch (op) {
                    case RegularOperator::UNION: {
                        for (auto ch: getChildren()) {
                            if (!ch->isEmpty()) {
                                return false;
                            }
                        }
                        return true;
                    }
                    case RegularOperator::CONCAT: {
                        for (auto ch: getChildren()) {
                            if (ch->isEmpty()) {
                                return true;
                            }
                        }
                        return false;
                    }
                    case RegularOperator::STAR: {
                        return children[0]->isEmpty();
                    }
                }
            };

            virtual void flatten() override {
                for (const auto& ch:children) {
                    ch->flatten();
                }
                std::vector<std::shared_ptr<RegNode>> newChildren;
                for (const auto &ch: children) {
                    std::shared_ptr<RegOperation> asOp = std::dynamic_pointer_cast<RegOperation>(ch);

                    // Child has same operator, append its children to this layer
                    if (asOp != nullptr && asOp->op == op) {
                        for (const auto &chch: asOp->children) {
                            newChildren.push_back(chch);
                        }
                    } else {
                        newChildren.push_back(ch);
                    }
                }
                children = newChildren;

                if(op == RegularOperator::CONCAT) {
                    std::vector<std::shared_ptr<RegNode>> newChildren;
                    std::vector<IEntry*> concatted;
                    for (const auto& ch: children) {
                        std::shared_ptr<RegWord> asWord = std::dynamic_pointer_cast<RegWord>(ch);
                        if (asWord != nullptr) {
                            for (auto w: asWord->word) {
                                concatted.push_back(w);
                            }
                        } else {
                            if (!concatted.empty()) {
                                Words::Word cword(std::move(concatted));
                                newChildren.push_back(std::make_shared<RegWord>(cword));
                                concatted = std::vector<IEntry*>{};
                            }
                            newChildren.push_back(ch);
                        }
                    }
                    if (!concatted.empty()) {
                        Words::Word cword(std::move(concatted));
                        newChildren.push_back(std::make_shared<RegWord>(cword));
                        concatted = std::vector<IEntry*>{};
                    }
                    if (newChildren.size() == 1) {
                        // Concat empty word to keep operation at least binary!
                        // Otherwise results in segfault
                        Words::Word emptyW;
                        RegWord emptyWord(emptyW);
                        newChildren.push_back(std::make_shared<RegWord>(emptyWord));
                    }
                    children = newChildren;
                }
            }

            std::shared_ptr<RegNode> derivative(std::string d) override {
                if (d.empty()) {
                    return std::make_shared<RegOperation>(op, children);
                }
                switch (op) {
                    case RegularOperator::UNION: {
                        std::vector<std::shared_ptr<RegNode>> chdDerivs{};
                        for (const auto &ch: getChildren()) {
                            std::shared_ptr<RegNode> deriv = ch->derivative(d);
                            if (std::dynamic_pointer_cast<RegEmpty>(deriv) == nullptr) {
                                chdDerivs.push_back(deriv);
                            }
                        }
                        if (chdDerivs.empty()) {
                            return std::make_shared<RegEmpty>(RegEmpty());
                        } else if (chdDerivs.size() == 1) {
                            return chdDerivs[0];
                        }
                        return std::make_shared<RegOperation>(RegOperation(RegularOperator::UNION, chdDerivs));
                        break;
                    }
                    case RegularOperator::CONCAT: {
                        if (children.size() > 2) {
                            std::vector<std::shared_ptr<RegNode>> lhsChild{};
                            for (unsigned i = 1; i < children.size(); i++) {
                                lhsChild.push_back(children[i]);
                            }
                            RegOperation lhs(RegularOperator::CONCAT, lhsChild);

                            std::vector<std::shared_ptr<RegNode>> binaryCh{children[0], std::make_shared<RegOperation>(lhs)};
                            RegOperation binary(RegularOperator::CONCAT, binaryCh);
                            return binary.derivative(d);
                        }

                        std::vector<std::shared_ptr<RegNode>> chdRev{};
                        auto lhs = getChildren()[0];
                        auto rhs = getChildren()[1];
                        auto newLhs = lhs->derivative(d);

                        // Check if newLhs is epsilon, then we're done here
                        auto asword = std::dynamic_pointer_cast<RegWord>(newLhs);
                        if (asword != nullptr && asword->word.characters() == 0) {
                                return rhs;
                        }
                        if (lhs->acceptsEpsilon()) {
                            auto newRhs = rhs->derivative(d);
                            std::vector<std::shared_ptr<RegNode>> conc1Vect{newLhs, rhs};
                            std::vector<std::shared_ptr<RegNode>> conc2Vect;

                            auto newRhsAsWord = std::dynamic_pointer_cast<RegWord>(newRhs);
                            if (newRhs == nullptr || newRhs->characters() > 0) {
                                conc2Vect.push_back(newRhs);
                            }

                            if(std::dynamic_pointer_cast<RegEmpty>(newLhs) != nullptr) {
                                return newRhs;
                            }

                            std::shared_ptr<RegNode> conc = std::make_shared<RegOperation>(RegularOperator::CONCAT, conc1Vect);
                            if(std::dynamic_pointer_cast<RegEmpty>(newRhs) != nullptr) {
                                return conc;
                            }
                            auto unionVec = std::vector<std::shared_ptr<RegNode>>{conc, newRhs};
                            return std::make_shared<RegOperation>(RegularOperator::UNION, unionVec);
                        } else {
                            std::vector<std::shared_ptr<RegNode>> chd{newLhs, rhs};
                            if(std::dynamic_pointer_cast<RegEmpty>(newLhs) != nullptr) {
                                return newLhs;
                            }
                            return std::make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT, chd));
                        }
                        break;
                    }
                    case RegularOperator::STAR: {
                        auto cpy = std::make_shared<RegOperation>(RegOperation(RegularOperator::STAR, children));
                        auto chdderiv = getChildren()[0]->derivative(d);

                        if (std::dynamic_pointer_cast<RegEmpty>(chdderiv)) {
                            return std::make_shared<RegEmpty>();
                        }

                        auto asWord = std::dynamic_pointer_cast<RegEmpty>(chdderiv) ;
                        if (asWord != nullptr && asWord->characters() == 0) {
                            return cpy;
                        }

                        std::vector<std::shared_ptr<RegNode>> chd{chdderiv, cpy};
                        return std::make_shared<RegOperation>(RegOperation(RegularOperator::CONCAT, chd));
                        break;
                    }
                }
            }

            RegularOperator getOperator() { return op; };

            void toString(std::ostream &os) override {
                switch (op) {
                    case RegularOperator::UNION:
                        os << "(";
                        for (unsigned i = 0; i < children.size() - 1; i++) {
                            children[i]->toString(os);
                            os << "|";
                        }
                        children[children.size() - 1]->toString(os);
                        os << ")";
                        break;
                    case RegularOperator::CONCAT:
                        os << "(";
                        for (unsigned i = 0; i < children.size(); i++) {
                            children[i]->toString(os);
                        }
                        os << ")";
                        break;
                    case RegularOperator::STAR:
                        os << "(";
                        for (unsigned i = 0; i < children.size(); i++) {
                            children[i]->toString(os);
                        }
                        os << ")*";
                        break;
                }
            }

            int shortestLiteral() override {
                int shortest = 0;
                if (op == RegularOperator::STAR) {
                    // star of shortest literals is shortest literal
                    return 0;
                } else  if (op == RegularOperator::CONCAT) {
                    // Concatenation of shortest literals is shortest literal
                    for (const auto& ch: children) {
                        shortest += ch->shortestLiteral();
                    }
                    return shortest;
                } else {
                    for (const auto& ch: children) {
                        int chl = ch->shortestLiteral();
                        if (chl < shortest) {
                            shortest = chl;
                        }
                    }
                    return shortest;
                }
            };

            virtual int numStars() override {
                int c = 0;
                if (op == RegularOperator::STAR) {
                    c++;
                }
                for (const auto& ch: children) {
                    c += ch->numStars();
                }
                return c;
            };

            virtual int starHeight() override {
                int c = 0;
                for (const auto &ch: children) {
                    int csh = ch->starHeight();
                    if (csh > c) {
                        c = csh;
                    }
                }

                if (op == RegularOperator::STAR) {
                    c++;
                }
                return c;
            }

            int longestLiteral() override {
                int longest = 0;

                if (op == RegularOperator::CONCAT) {
                    // Concatenation of longest literals is longest literal
                    for (const auto& ch: children) {
                        longest += ch->longestLiteral();
                    }
                    return longest;
                }

                for (const auto& ch: children) {
                    int chl = ch->longestLiteral();
                    if (chl > longest) {
                        longest = chl;
                    }
                }
                return longest;
            }

            size_t characters() override {
                int sum = 0;
                for (auto c: children) {
                    sum += c->characters();
                }
                return sum;
            }

            long complexity() override {
                int sum = 0;
                for (const auto& c: children) {
                    sum += c->complexity();
                }
                return sum+1;
            }

            void getAlphabet(Words::WordBuilder wb) override {
                for (auto c: children) {
                    c->getAlphabet(wb);
                }
            }

            int depth() override {
                int d = 0;
                for (const auto& c: children) {
                    int cd = c->depth();
                    if (cd > d) {
                        d = cd;
                    }
                }
                return d+1;
            };

            /**
             * Returns a vector of the operand nodes.
             */
            std::vector<std::shared_ptr<RegNode>> getChildren() {
                return children;
            };

            bool acceptsEpsilon() override {
                switch (op) {
                    case RegularOperator::UNION: {

                        for (auto ch: getChildren()) {
                            if (ch->acceptsEpsilon()) {
                                return true;
                            }
                        }
                        return false;
                        break;
                    }
                    case RegularOperator::CONCAT: {
                        for (auto ch: getChildren()) {
                            if (!ch->acceptsEpsilon()) {
                                return false;
                            }
                        }
                        return true;
                    }
                    case RegularOperator::STAR: {
                        return true;
                    }
                }
            }

        private:
            RegularOperator op;
            std::vector<std::shared_ptr<RegNode>> children;
        };




        class RegConstraint {
        public:
            RegConstraint(Words::Word pattern, std::shared_ptr<RegNode> expr) : pattern(pattern), expr(expr) {};

            void toString(std::ostream &os) {
                os << pattern;
                os << "⋵ ";
                expr->toString(os);
            }

            Words::Word pattern;
            std::shared_ptr<RegNode> expr;
        };


    }  // namespace RegularConstraints
}  // namespace Words
