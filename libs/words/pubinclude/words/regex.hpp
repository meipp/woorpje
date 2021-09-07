#include <string>
#include<vector>
#include "smtparser/ast.hpp"


namespace Words {
    namespace Regex {
        /**
         * ABC for regular expression trees.
         */
        class RegNode {
        public:
            //virtual RegNode deepCopy();
            virtual ~RegNode() {}
            virtual void toString(std::ostream& os) = 0;
            virtual bool isLeaf() {return false;}
        };

        /**
         * Current supported regular operators.
         */
        enum class RegularOperator { UNION, CONCAT, STAR };

        /**
         * Node representing a regular operation.
         * Owns a list of children as the operands.
         */
        class RegOperation : public RegNode {
        public:
            RegOperation(RegularOperator regOperator)
                : RegOperation(regOperator, std::vector<std::shared_ptr<RegNode>> {}){};
            RegOperation(RegularOperator regOperator, std::vector<std::shared_ptr<RegNode>> children)
                : op(regOperator), children(children){};

            /**
             * Adds a child (i.e. operand) to this node.
             */
            void addChild(std::shared_ptr<RegNode> node){
                children.push_back(node);
            };
            virtual bool isLeaf() { return false; }

            void toString(std::ostream& os) {
                switch(op) {
                    case RegularOperator::UNION:
                        os << "(";
                        for (int i = 0; i < children.size()-1; i++) {
                            children[i]->toString(os);
                            os << "|";
                        }
                        children[children.size()-1]->toString(os);
                        os << ")";
                        break;
                    case RegularOperator::CONCAT:
                        os << "(";
                        for (int i = 0; i < children.size(); i++) {
                            children[i]->toString(os);
                        }
                        os << ")";
                        break;
                    case RegularOperator::STAR:
                        os << "(";
                        for (int i = 0; i < children.size(); i++) {
                            children[i]->toString(os);
                        }
                        os << ")*";
                        break;
                }
            }

            /**
             * Returns a vector of the operand nodes.
             */
            std::vector<std::shared_ptr<RegNode>> getChildren() {
                return children;
            };
        private:
            RegularOperator op;
            std::vector<std::shared_ptr<RegNode>> children;
        };

        class RegWord : public RegNode {
        public:
            //RegWord(std::string word) : word(word){};
            RegWord(Words::Word word): word(word){};

            void toString(std::ostream& os) {
                for (auto c: word) 
                    os << c->getTerminal()->getChar();
            }

            virtual bool isLeaf() { return true; }

        private:
            Words::Word word;
        };

        class RegEmpty : public RegNode {
        public:
            RegEmpty(){};
            virtual bool isLeaf() { return true; }

            void toString(std::ostream& os) {
                os << "</>";
                
            }

        };


    }  // namespace Regex
}  // namespace Words
