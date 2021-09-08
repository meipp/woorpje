#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <iostream>
#include "words/regconstraints.hpp"
#include "words/words.hpp"
#include "core/Solver.h"


namespace RegularEncoding {

    namespace Automaton {

        class NFA {
        private:
        std::vector<int> epsilon_closure(int);
        std::vector<int> states;
        std::map<int, std::tuple<std::string, int>> delta;
        int q0;
        std::vector<int> final_states;

        public:
        NFA(/* args */);
        ~NFA();
        bool accept(std::string);
        void remove_epsilon_transitions();
        void get_reachable_states();
        int new_state();
        void set_initial_state(int);
        void add_final_state(int);
        void add_transition(int, std::string, int);
        void remove_transition(int, std::string, int);
        std::map<int, int> merge_automaton(NFA);
        };

        NFA regex_to_nfa(std::string);

        } // namespace Automaton

    namespace PropositionalLogic {

        enum class Junctor { AND = 1, OR = 2, NOT = 3, LIT = 0 };

        class PLFormula {
        private:
        PLFormula(Junctor, std::vector<PLFormula>);
        explicit PLFormula(int); // Literal formula
        //~PLFormula();

        Junctor junctor;
        int literal;
        std::vector<PLFormula> subformulae;

        public:
        static PLFormula land(std::vector<PLFormula>);
        static PLFormula lor(std::vector<PLFormula>);
        static PLFormula lnot(PLFormula);
        static PLFormula lit(int);

        bool valid();
        bool is_nenf();
        int depth();
        void make_binary();
        int max_var();

        Junctor getJunctor() { return junctor; }

        std::vector<PLFormula> getSubformulae() { return subformulae; }

        int getLiteral() const { return literal; }
        };

        std::set<std::set<int>> tseytin_cnf(PLFormula);

    } // namespace PropositionalLogic

    class Encoder {
    public:
        Encoder(Words::RegularConstraints::RegConstraint constraint, 
            std::map<Words::Variable *, int>* vIndices,
            std::map<Words::Terminal *, int>* tIndices,
            std::map<std::pair<std::pair<int, int>, int>, Glucose::Var>* variableVars,
            std::map<std::pair<int, int>, Glucose::Var>* constantsVars):
            constraint(std::move(constraint)), vIndices(vIndices), tIndices(tIndices), variableVars(variableVars), constantsVars(constantsVars) {};
        virtual ~Encoder(){};

        virtual std::set<std::set<int>> encode(){};
    
    protected:
        Words::RegularConstraints::RegConstraint constraint;
        std::map<Words::Variable *, int>* vIndices;
        std::map<Words::Terminal *, int>* tIndices;
        std::map<std::pair<std::pair<int, int>, int>, Glucose::Var>* variableVars;
        std::map<std::pair<int, int>, Glucose::Var>* constantsVars;
    };

    class InductiveEncoder : public Encoder {
    public:    
        InductiveEncoder(Words::RegularConstraints::RegConstraint constraint,
            std::map<Words::Variable *, int>* vIndices,
            std::map<Words::Terminal *, int>* tIndices,
            std::map<std::pair<std::pair<int, int>, int>, Glucose::Var>* variableVars,
            std::map<std::pair<int, int>, Glucose::Var>* constantsVars)
            : Encoder(constraint, vIndices, tIndices, variableVars, constantsVars) {
            };
        std::set<std::set<int>> encode();

    private:
        PropositionalLogic::PLFormula doEncode(Words::Word pat, std::shared_ptr<Words::RegularConstraints::RegNode> expression);
        PropositionalLogic::PLFormula encodeWord(Words::Word pat, std::shared_ptr<Words::RegularConstraints::RegWord> expression);
        PropositionalLogic::PLFormula encodeUnion(Words::Word pat, std::shared_ptr<Words::RegularConstraints::RegOperation> expression);
        PropositionalLogic::PLFormula encodeConcat(Words::Word pat, std::shared_ptr<Words::RegularConstraints::RegOperation> expression);
        PropositionalLogic::PLFormula encodeStar(Words::Word pat, std::shared_ptr<Words::RegularConstraints::RegOperation> expression);
    };

    class AutomatonEncoder : public Encoder {
        using Encoder::Encoder;
    };

} // namespace RegularEncoding