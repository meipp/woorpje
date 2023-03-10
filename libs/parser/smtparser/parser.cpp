#include <sstream>
#include <iostream>
#include <unordered_map>
#include <typeinfo>

#include "parser/parsing.hpp"
#include "smtparser/parser.hpp"
#include "smtparser/ast.hpp"
#include "words/linconstraint.hpp"
#include "words/exceptions.hpp"
#include "words/regconstraints.hpp"

#include "core/Solver.h"

#include "util.hpp"

namespace Words {

    class JGenerator : public Words::JobGenerator {
    public:
        JGenerator() {}
        virtual ~JGenerator() = default;

        std::unique_ptr<Words::Job> newJob() {

            std::stringstream str;

            if (solver.solve(assumptions)) {
                UpdateSolverBuilder builder(hashToLit, constraints, eqs, solver, neqmap, assumptions);
                for (auto &t: parser.getAssert()) {
                    builder.Run(*t);
                }

                auto job = builder.finalise();
                job->options.context = context;

                // Copy regular constraint
                std::vector<std::shared_ptr<Words::RegularConstraints::RegConstraint>> rcp{};
                for (Words::RegularConstraints::RegConstraint rc: recons) {
                    auto shrd = std::make_shared<Words::RegularConstraints::RegConstraint>(std::move(rc));
                    rcp.push_back(shrd);
                }

                //auto mSharedPtr = std::make_shared<std::vector<Words::RegularConstraints::RegConstraint>>(std::move(recons));
                job->options.recons = rcp;

                return job;
            } else {
                return nullptr;
            }
        }

        std::shared_ptr<Words::Context> context = nullptr;
        Glucose::Solver solver;
        Glucose::vec<Glucose::Lit> assumptions;
        std::unordered_map<Glucose::Var, Words::Equation> eqs;
        std::unordered_map<Glucose::Var, Words::Constraints::Constraint_ptr> constraints;
        std::unordered_map<size_t, Glucose::Lit> hashToLit;
        std::vector<Words::RegularConstraints::RegConstraint> recons;
        SMTParser::Parser parser;
        std::unordered_map<size_t, ASTNode_ptr> neqmap;
    };


    template<typename T>
    class AutoNull {
    public:
        AutoNull(std::unique_ptr<T> &ptr) : ptr(ptr) {}

        ~AutoNull() { ptr = nullptr; }

    private:
        std::unique_ptr<T> &ptr;
    };


    /**
     * Adds terminal occurring terminal symbols to the context's alphabet
     */
    class TerminalAdder : public BaseVisitor {
    public:
        TerminalAdder(Words::Context &opt) : ctxt(opt) {}

        virtual void caseStringLiteral(StringLiteral &s) {
            for (auto c: s.getVal()) {
                Words::IEntry *entry;
                try {
                    entry = ctxt.findSymbol(c);
                }
                catch (Words::WordException &e) {
                    entry = ctxt.addTerminal(c);
                }
                assert(entry);
                if (entry->isVariable()) {
                    throw UnsupportedFeature();
                }
            }
        }

        virtual void caseAssert(Assert &c) {
            c.getExpr()->accept(*this);
        }

        virtual void caseFunctionApplication(FunctionApplication &c) {
            std::cerr << c << std::endl;
            throw UnsupportedFeature();
        }

    private:
        Words::Context &ctxt;
    };


    class LengthConstraintBuilder : public BaseVisitor {
    public:
        LengthConstraintBuilder(
                Words::Context &ctxt,
                Glucose::Solver &solver,
                std::unordered_map<Glucose::Var, Words::Constraints::Constraint_ptr> &c,
                std::unordered_map<Glucose::Var, Words::Equation> &eqs,
                std::unordered_map<size_t, Glucose::Lit> &hlit,
                std::unordered_map<size_t, ASTNode_ptr> &neqmap
        ) : ctxt(ctxt),
            solver(solver),
            constraints(c),
            eqs(eqs),
            alreadyCreated(hlit),
            neqmap(neqmap) {}

        template<Words::Constraints::Cmp cmp>
        void visitRedirect(DummyApplication &c) {
            if (!checkAlreadyIn(c, var)) {
                var = Glucose::lit_Undef;
                if (c.getExpr(0)->getSort() == Sort::Integer) {
                    builder = Words::Constraints::makeLinConstraintBuilder(cmp);
                    adder = std::make_unique<Adder<Words::Constraints::LinearConstraintBuilder> >(builder);
                    AutoNull<Adder<Words::Constraints::LinearConstraintBuilder> > nuller(adder);
                    c.getExpr(0)->accept(*this);
                    adder->switchSide();
                    c.getExpr(1)->accept(*this);


                    auto v = solver.newVar();
                    var = Glucose::mkLit(v);
                    auto cc = builder->makeConstraint();
                    if (cc->getLinconstraint()->unsatisfiable())
                        solver.addClause(~var);
                    constraints.insert(std::make_pair(v, std::move(cc)));
                    builder = nullptr;
                    insert(c, var);
                }
            }
        }

        void Run(ASTNode &m) {
            m.accept(*this);
        }

        virtual void caseLEQ(LEQ &c) override {
            visitRedirect<Words::Constraints::Cmp::LEq>(c);
        }

        virtual void caseLT(LT &c) override {
            visitRedirect<Words::Constraints::Cmp::Lt>(c);
        }

        virtual void caseGEQ(GEQ &c) override {
            visitRedirect<Words::Constraints::Cmp::GEq>(c);
        }

        virtual void caseGT(GT &c) override {
            visitRedirect<Words::Constraints::Cmp::Gt>(c);
        }

        bool emptyWord(Words::Word &left) {
            return left.noVariableWord() && left.noTerminalWord();
        }

	/* HOL functions */ 
        virtual void caseStrPrefixOf(StrPrefixOf &cc) override {
                // lexpr is a prefix of rexpr
                var = Glucose::lit_Undef;
                auto lexpr = cc.getExpr(0);
                auto rexpr = cc.getExpr(1);

                // build prefix string
                static size_t i = 0;
                std::stringstream str;
                str << "_woorpje_tmp_prefix" << i;
                auto symb_pref = std::make_shared<Symbol>(str.str());
                symb_pref->setSort(Sort::String);
                ctxt.addTemporaryVariable(str.str());

		i++;

                ASTNode_ptr new_var = std::make_shared<Identifier>(*symb_pref);
                auto lhs  = std::make_shared<StrConcat>(std::initializer_list<ASTNode_ptr>({lexpr, new_var}));
		auto rhs = std::make_shared<StrConcat>(std::initializer_list<ASTNode_ptr>({rexpr}));
		auto c = EQ(std::initializer_list<ASTNode_ptr>({lhs,rhs}));
	

		if (!checkAlreadyIn(cc, var)) {
                // build word equation
		  var = makeEquLit(cc);
		  
		  Words::Word left;
		  Words::Word right;
		  AutoNull<Words::WordBuilder> nuller(wb);
		  wb = ctxt.makeWordBuilder(left);
		  lhs->accept(*this);
		  wb->flush();
		  wb = ctxt.makeWordBuilder(right);
		  rhs->accept(*this);
		  wb->flush();
		  
		  Words::Equation eq(left, right);
		  eq.ctxt = &ctxt;
		  eqs.insert(std::make_pair(Glucose::var(var), eq));
		  
		  insert(cc, var);
		  
		  if (emptyWord(left) &&
		      !right.noTerminalWord()) {
                    solver.addClause(~var);
		  }
		  
		  if (emptyWord(right) &&
		      !left.noTerminalWord()) {
                    solver.addClause(~var);
		  }
		}
        }

	virtual void caseStrSuffixOf(StrSuffixOf &cc) override {
                var = Glucose::lit_Undef;
                auto lexpr = cc.getExpr(0);
                auto rexpr = cc.getExpr(1);
                static size_t i = 0;
                std::stringstream str;
                str << "_woorpje_tmp_suffix" << i;
                auto symb_suff = std::make_shared<Symbol>(str.str());
                symb_suff->setSort(Sort::String);
                ctxt.addTemporaryVariable(str.str());
                i++;

                ASTNode_ptr new_var = std::make_shared<Identifier>(*symb_suff);
                auto lhs  = std::make_shared<StrConcat>(std::initializer_list<ASTNode_ptr>({new_var, lexpr}));
                auto rhs = std::make_shared<StrConcat>(std::initializer_list<ASTNode_ptr>({rexpr}));
                auto c = EQ(std::initializer_list<ASTNode_ptr>({lhs,rhs}));


                if (!checkAlreadyIn(cc, var)) {
                // build word equation
                  var = makeEquLit(cc);

                  Words::Word left;
                  Words::Word right;
                  AutoNull<Words::WordBuilder> nuller(wb);
                  wb = ctxt.makeWordBuilder(left);
                  lhs->accept(*this);
                  wb->flush();
                  wb = ctxt.makeWordBuilder(right);
                  rhs->accept(*this);
                  wb->flush();

                  Words::Equation eq(left, right);
                  eq.ctxt = &ctxt;
                  eqs.insert(std::make_pair(Glucose::var(var), eq));

                  insert(cc, var);

                  if (emptyWord(left) &&
                      !right.noTerminalWord()) {
                    solver.addClause(~var);
                  }

                  if (emptyWord(right) &&
                      !left.noTerminalWord()) {
                    solver.addClause(~var);
                  }
                }
	}

	virtual void caseStrContains(StrContains &cc) override {
                var = Glucose::lit_Undef;
                auto lexpr = cc.getExpr(0);
                auto rexpr = cc.getExpr(1);
                static size_t i = 0;
                std::stringstream str;
                str << "_woorpje_tmp_contains_l" << i;
                auto symb_contains_l = std::make_shared<Symbol>(str.str());
                symb_contains_l->setSort(Sort::String);
                ctxt.addTemporaryVariable(str.str());
		str.str("");
		str << "_woorpje_tmp_contains_r" << i;
                auto symb_contains_r = std::make_shared<Symbol>(str.str());
                symb_contains_r->setSort(Sort::String);
                ctxt.addTemporaryVariable(str.str());
		i++;

                ASTNode_ptr new_var_l = std::make_shared<Identifier>(*symb_contains_l);
		ASTNode_ptr new_var_r = std::make_shared<Identifier>(*symb_contains_r);
		auto lhs  = std::make_shared<StrConcat>(std::initializer_list<ASTNode_ptr>({new_var_l, lexpr, new_var_r}));
                auto rhs = std::make_shared<StrConcat>(std::initializer_list<ASTNode_ptr>({rexpr}));
                auto c = EQ(std::initializer_list<ASTNode_ptr>({lhs,rhs}));


                if (!checkAlreadyIn(cc, var)) {
                // build word equation
                  var = makeEquLit(cc);

                  Words::Word left;
                  Words::Word right;
                  AutoNull<Words::WordBuilder> nuller(wb);
                  wb = ctxt.makeWordBuilder(left);
                  lhs->accept(*this);
                  wb->flush();
                  wb = ctxt.makeWordBuilder(right);
                  rhs->accept(*this);
                  wb->flush();

                  Words::Equation eq(left, right);
                  eq.ctxt = &ctxt;
                  eqs.insert(std::make_pair(Glucose::var(var), eq));

                  insert(cc, var);

                  if (emptyWord(left) &&
                      !right.noTerminalWord()) {
                    solver.addClause(~var);
                  }

                  if (emptyWord(right) &&
                      !left.noTerminalWord()) {
                    solver.addClause(~var);
                  }
                }
        }

    
		
		
	/**
         * Adds word equations to the job's context.
         */
        virtual void caseEQ(EQ &c) override {
            if (!checkAlreadyIn(c, var)) {
                var = Glucose::lit_Undef;
                auto lexpr = c.getExpr(0);
                auto rexpr = c.getExpr(1);
                if (lexpr->getSort() == Sort::String) {
                    var = makeEquLit(c);

                    Words::Word left;
                    Words::Word right;
                    AutoNull<Words::WordBuilder> nuller(wb);
                    wb = ctxt.makeWordBuilder(left);
                    lexpr->accept(*this);
                    wb->flush();
                    wb = ctxt.makeWordBuilder(right);
                    rexpr->accept(*this);
                    wb->flush();
                    //opt.equations.emplace_back(left,right);
                    //opt.equations.back().ctxt = opt.context.get();

                    Words::Equation eq(left, right);
                    eq.ctxt = &ctxt;
                    var = makeEquLit(c);
                    eqs.insert(std::make_pair(Glucose::var(var), eq));

                    insert(c, var);

                    if (emptyWord(left) &&
                        !right.noTerminalWord()) {
                        solver.addClause(~var);
                    }

                    if (emptyWord(right) &&
                        !left.noTerminalWord()) {
                        solver.addClause(~var);
                    }


                } else if (lexpr->getSort() == Sort::Integer) {
                    //Throw for now,  as the visitRedirect cannot handle LEQ and GEQ simultaneously
                    throw UnsupportedFeature();
                    visitRedirect<Words::Constraints::Cmp::LEq>(c);
                    visitRedirect<Words::Constraints::Cmp::GEq>(c);
                } else if (lexpr->getSort() == Sort::Bool) {
                    throw Words::WordException("SHouldn't get here");
                }
            }
        }

        Glucose::Lit makeEquLit(ASTNode& c) {
            if (equalities.count(&c)) {
                return equalities.at(&c);
            }
            auto v = solver.newVar();
            auto var = Glucose::mkLit(v);
            equalities.insert(std::make_pair(&c, var));

            return var;
        }

        virtual void caseNEQ(NEQ &c) override {
            if (!checkAlreadyIn(c, var)) {
                var = Glucose::lit_Undef;
                auto lexpr = c.getExpr(0);
                auto rexpr = c.getExpr(1);
                if (lexpr->getSort() == Sort::String) {
                    auto eqLit = makeEquLit(*c.getInnerEQ());
                    //WE need to grab the inner equality,
                    //negate the literal associated to it (pass that upwards)
                    //and add a biimplication to the disjunction created below.

                    static size_t i = 0;

                    std::stringstream str;
                    str << "_woorpje_diseq_pref" << i;
                    auto symb_pref = std::make_shared<Symbol>(str.str());
                    ctxt.addVariable(str.str());
                    str.str("");
                    str << "_woorpje_diseq_suf_l" << i;
                    auto symb_sufl = std::make_shared<Symbol>(str.str());
                    ctxt.addVariable(str.str());
                    str.str("");
                    str << "_woorpje_diseq_suf_r" << i;
                    i++;
                    auto symb_sufr = std::make_shared<Symbol>(str.str());
                    ctxt.addVariable(str.str());


                    std::vector<ASTNode_ptr> disjuncts;
                    if (ctxt.getTerminalAlphabet().size() > 2) {
                        //Greater than 2, because it only makes to distinguish the middle character,
                        //when we have more than an epsilon and one extra character in the terminals
                        symb_pref->setSort(Sort::String);
                        symb_sufl->setSort(Sort::String);
                        symb_sufr->setSort(Sort::String);


                        ASTNode_ptr Z = std::make_shared<Identifier>(*symb_pref);
                        ASTNode_ptr X = std::make_shared<Identifier>(*symb_sufl);
                        ASTNode_ptr Y = std::make_shared<Identifier>(*symb_sufr);


                        for (auto a: ctxt.getTerminalAlphabet()) {
                            if (a->isEpsilon()) {
                                continue;
                            }
                            std::vector<ASTNode_ptr> innerdisjuncts;
                            ASTNode_ptr strl = std::make_shared<StringLiteral>(std::string(1, a->getRepr()));
                            auto concat = std::make_shared<StrConcat>(std::initializer_list<ASTNode_ptr>({Z, strl, X}));
                            ASTNode_ptr outeq = std::make_shared<EQ>(
                                    std::initializer_list<ASTNode_ptr>({lexpr, concat}));
                            for (auto b: ctxt.getTerminalAlphabet()) {
                                if (b == a || b->isEpsilon()) {
                                    continue;
                                }
                                ASTNode_ptr strr = std::make_shared<StringLiteral>(std::string(1, b->getRepr()));
                                ASTNode_ptr concat_nnner = std::make_shared<StrConcat>(
                                        std::initializer_list<ASTNode_ptr>({Z, strr, Y}));
                                ASTNode_ptr in_eq = std::make_shared<EQ>(
                                        std::initializer_list<ASTNode_ptr>({rexpr, concat_nnner}));
                                innerdisjuncts.push_back(in_eq);
                            }
                            ASTNode_ptr disj = std::make_shared<Disjunction>(std::move(innerdisjuncts));
                            disjuncts.push_back(
                                    std::make_shared<Conjunction>(std::initializer_list<ASTNode_ptr>({outeq, disj})));

                        }
                    }


                    ASTNode_ptr llength = std::make_shared<StrLen>(std::initializer_list<ASTNode_ptr>({lexpr}));
                    ASTNode_ptr rlength = std::make_shared<StrLen>(std::initializer_list<ASTNode_ptr>({rexpr}));
                    ASTNode_ptr gt = std::make_shared<GT>(std::initializer_list<ASTNode_ptr>({llength, rlength}));
                    ASTNode_ptr lt = std::make_shared<LT>(std::initializer_list<ASTNode_ptr>({llength, rlength}));

                    disjuncts.push_back(gt);
                    disjuncts.push_back(lt);


                    ASTNode_ptr outdisj = std::make_shared<Disjunction>(std::move(disjuncts));
                    outdisj->accept(*this);

                    Glucose::vec<Glucose::Lit> clause;
                    clause.push(~eqLit);
                    reify_and_bi(solver, var, clause);
                    insert(c, var);
                    neqmap.insert(std::make_pair(c.hash(), outdisj));
                } else {
                    throw Words::UnsupportedFeature();
                }
            }
        }

        virtual void caseNumericLiteral(NumericLiteral &c) override {
            if (!vm) {
                adder->add(c.getVal());
            } else {
                vm->number = c.getVal();
            }
        }

        virtual void caseFunctionApplication(FunctionApplication&) override {
            throw UnsupportedFeature();
        }

        virtual void caseMultiplication(Multiplication &c) override {
            Words::Constraints::VarMultiplicity kk(nullptr, 1);
            vm = &kk;
            for (auto &cc: c) {
                cc->accept(*this);
            }
            assert(kk.entry);
            adder->add(kk);
            vm = nullptr;
        }

        virtual void caseStringLiteral(StringLiteral &s) override {
            if (adder) {
                adder->add(s.getVal().size());
            } else {
                for (auto c: s.getVal()) {
                    *wb << c;
                }
            }
        }

        virtual void caseIdentifier(Identifier &c) override {
            if (c.getSort() == Sort::String && instrlen) {
                if (vm) {
                    vm->entry = ctxt.findSymbol(c.getSymbol()->getVal());
                } else {
                    entry = ctxt.findSymbol(c.getSymbol()->getVal());
                }
            } else if (c.getSort() == Sort::String && !instrlen) {
                auto symb = c.getSymbol();
                *wb << symb->getVal();
            } else if (c.getSort() == Sort::Integer) {
                UnsupportedFeature();
            } else if (c.getSort() == Sort::Bool) {
                if (!checkAlreadyIn(c, var)) {
                    if (boolvar.count(&c)) {
                        var = boolvar.at(&c);
                    } else {
                        auto v = solver.newVar();
                        var = Glucose::mkLit(v);
                        boolvar.insert(std::make_pair(&c, var));
                    }
                    insert(c, var);
                }
            } else {
                throw Words::WordException("Error hh");
            }
        }

        virtual void caseNegLiteral(NegLiteral &c) override {
            if (!checkAlreadyIn(c, var)) {
                c.inner()->accept(*this);
                var = ~var;
                insert(c, var);
            }
        }

        virtual void caseAssert(Assert &c) override {
            c.getExpr()->accept(*this);
            if (var != Glucose::lit_Undef) {
                solver.addClause(var);
            }
        }

        virtual void caseStrLen(StrLen &c) override {
            instrlen = true;
            c.getExpr(0)->accept(*this);
            instrlen = false;
            if (entry && !vm) {
                Words::Constraints::VarMultiplicity kk(entry, 1);
                adder->add(kk);
                entry = nullptr;
            }
        }

        virtual void caseStrConcat(StrConcat &c) override {
            for (auto &cc: c) {
                cc->accept(*this);
                if (instrlen) {
                    if (entry) {
                        Words::Constraints::VarMultiplicity kk(entry, 1);
                        adder->add(kk);
                        entry = nullptr;
                    }
                }
            }
        }

        virtual void caseDisjunction(Disjunction &c) override {
            if (!checkAlreadyIn(c, var)) {
                Glucose::vec<Glucose::Lit> vec;
                for (auto cc: c) {
                    cc->accept(*this);
                    if (var != Glucose::lit_Undef)
                        vec.push(var);
                }
                if (vec.size()) {
                    auto v = solver.newVar();
                    var = Glucose::mkLit(v);
                    reify_or_bi(solver, var, vec);

                    insert(c, var);
                } else {
                    var = Glucose::lit_Undef;
                }
            }
        }

        virtual void caseConjunction(Conjunction &c) override {
            if (!checkAlreadyIn(c, var)) {
                Glucose::vec<Glucose::Lit> vec;
                for (auto cc: c) {
                    cc->accept(*this);
                    if (var != Glucose::lit_Undef)
                        vec.push(var);
                }
                if (vec.size()) {
                    auto v = solver.newVar();
                    var = Glucose::mkLit(v);
                    reify_and_bi(solver, var, vec);
                    insert(c, var);
                } else {
                    var = Glucose::lit_Undef;
                }
            }
        }

        virtual void caseReIn(ReIn&) override {
            // Do nothing
        }

    private:

        bool checkAlreadyIn(ASTNode &n, Glucose::Lit &l) {
            l = Glucose::lit_Undef;
            auto h = n.hash();
            if (alreadyCreated.count(h)) {
                l = alreadyCreated.at(h);
                return true;
            }
            return false;
        }

        void insert(ASTNode &n, Glucose::Lit &l) {
            alreadyCreated.insert(std::make_pair(n.hash(), l));
        }

        Words::Context &ctxt;
        std::unique_ptr<Words::Constraints::LinearConstraintBuilder> builder;
        Words::Constraints::VarMultiplicity *vm = nullptr;
        std::unique_ptr<Adder<Words::Constraints::LinearConstraintBuilder>> adder;
        IEntry *entry = nullptr;
        Glucose::Solver &solver;
        std::unordered_map<Glucose::Var, Words::Constraints::Constraint_ptr> &constraints;
        Glucose::Lit var = Glucose::lit_Undef;

        std::unique_ptr<WordBuilder> wb;
        std::unordered_map<Glucose::Var, Words::Equation> &eqs;
        std::unordered_map<void *, Glucose::Lit> boolvar;

        std::unordered_map<ASTNode *, Glucose::Lit> equalities;

        bool instrlen = false;
        std::unordered_map<size_t, Glucose::Lit> &alreadyCreated;
        std::unordered_map<size_t, ASTNode_ptr> &neqmap;
    };

    class RegularConstraintBuilder : public BaseVisitor {
    public:
        RegularConstraintBuilder(Words::Context &ctxt, std::vector<Words::RegularConstraints::RegConstraint>& reconstraints) : ctxt(ctxt), reconstraints(reconstraints) {}

        virtual void caseAssert(Assert &c) {
            c.getExpr()->accept(*this);
        }

        /**
         * Base method for parsing the predicate.
         */
        virtual void caseReIn(ReIn &c) {

            Words::Word pattern;

            // Without this: Segmentation fault, why??
            AutoNull<Words::WordBuilder> nuller(wordBuilder);

            wordBuilder = ctxt.makeWordBuilder(pattern);
            c.getExpr(0)->accept(*this);
            wordBuilder->flush();
            
            ASTNode_ptr expression = c.getExpr(1);
            if (!expression) {
                throw Words::WordException("Invalid predicate");
            }

            expression->accept(*this);


            Words::RegularConstraints::RegConstraint constraint(pattern, root);
            std::stringstream ss;
            constraint.toString(ss);
            std::cout << "\tParsed Regular Constraint:  " << ss.str() << "\n";


            reconstraints.push_back(constraint);
        }

        virtual void caseStringLiteral(StringLiteral &s) {
            for (auto c: s.getVal()) {
                assert (wordBuilder != nullptr);
                *wordBuilder << c;
            }
        }

        // Pattern
        virtual void caseStrConcat(StrConcat &c) {
            for (auto &cc: c) {
                cc->accept(*this);
            }
        }

        virtual void caseIdentifier(Identifier &c) {
            auto symb = c.getSymbol();
            *wordBuilder << symb->getVal();
        }



        // Re
        virtual void caseReTo(ReTo &c) {
            Words::Word w;
            wordBuilder = ctxt.makeWordBuilder(w);

            for (auto &cc: c)
                cc->accept(*this);

            wordBuilder->flush();
            std::shared_ptr<Words::RegularConstraints::RegWord> rword(new Words::RegularConstraints::RegWord(w));

            if (root == nullptr) {
                root = rword;
            } else {
                parent->addChild(rword);
            }

            // No need to set parent, is leaf.

        }

        virtual void caseReLoop(ReLoop& s) {
            // Inline to finite union of concatenations
            int lower = s.lower->getVal();
            int upper = s.upper->getVal();
            if (lower > upper) {
                // <=> re.none
                auto none = std::make_shared<Words::RegularConstraints::RegEmpty>();
                if (root == nullptr) {
                    root = none;
                } else {
                    parent->addChild(none);
                }
            } else if (lower == upper) {
                // <=> e^n            
                std::shared_ptr<Words::RegularConstraints::RegOperation> concat(new Words::RegularConstraints::RegOperation(Words::RegularConstraints::RegularOperator::CONCAT));
                if (root == nullptr) {
                    root = concat;
                } else {
                    parent->addChild(concat);
                }
                for (int i = 0; i<lower;i++) {
                    parent = concat;
                    s.getExpr(0)->accept(*this);
                }
            } else {
                std::shared_ptr<Words::RegularConstraints::RegOperation> tlunion(new Words::RegularConstraints::RegOperation(Words::RegularConstraints::RegularOperator::UNION));
                if (root == nullptr) {
                    root = tlunion;
                } else {
                    parent->addChild(tlunion);
                }
                for (int i = lower; i <= upper; i++) {
                    parent = tlunion;
                    std::shared_ptr<Words::RegularConstraints::RegOperation> concat(new Words::RegularConstraints::RegOperation(Words::RegularConstraints::RegularOperator::CONCAT));
                    parent->addChild(concat);
                    for(int j = 0; j <i; j++) {
                        parent = concat;
                        s.getExpr(0)->accept(*this);
                    }
                }
            }

        }

        virtual void caseReUnion(ReUnion &c) {
            processRegOp(c, Words::RegularConstraints::RegularOperator::UNION);
        }

        virtual void caseReConcat(ReConcat &c) {
            processRegOp(c, Words::RegularConstraints::RegularOperator::CONCAT);
        }

        virtual void caseReStar(ReStar &c) {
            processRegOp(c, Words::RegularConstraints::RegularOperator::STAR);
        }

        virtual void caseReOpt(ReOpt &c) override {
            // (re.opt E) <=> (re.union E "")
            std::shared_ptr<Words::RegularConstraints::RegOperation> reunion(new Words::RegularConstraints::RegOperation(Words::RegularConstraints::RegularOperator::UNION));
            auto eps = std::make_shared<Words::RegularConstraints::RegWord>(Words::Word());
            reunion->addChild(eps);
            if (root == nullptr) {
                root = reunion;
            } else {
                parent->addChild(reunion);
            }
            parent = reunion;
            for (auto& cc: c) {
                cc->accept(*this);
            }
        }

        virtual void caseRePlus (RePlus &c) override {
            // (re.+ E) <=> (re.++ E (re.* E))
            std::shared_ptr<Words::RegularConstraints::RegOperation> reconcat(new Words::RegularConstraints::RegOperation(Words::RegularConstraints::RegularOperator::CONCAT));
            if (root == nullptr) {
                root = reconcat;
            } else {
                parent->addChild(reconcat);
            }
            parent = reconcat;
            for (auto& cc: c) {
                cc->accept(*this);
            }
            std::shared_ptr<Words::RegularConstraints::RegOperation> restar(new Words::RegularConstraints::RegOperation(Words::RegularConstraints::RegularOperator::STAR));
            reconcat->addChild(restar);
            parent = restar;
            for (auto& cc: c) {
                cc->accept(*this);
            }
        }

        virtual void caseReRange(ReRange& c){
            // Rewrite as union
            Words::Word from, to;
            wordBuilder = ctxt.makeWordBuilder(from);
            c.getExpr(0)->accept(*this);
            wordBuilder->flush();
            wordBuilder = ctxt.makeWordBuilder(to);
            c.getExpr(1)->accept(*this);
            wordBuilder->flush();

            if (!from.noVariableWord() || !to.noVariableWord()) {
                throw UnsupportedFeature("Only constants allowed in re.range");
            }

            if (from.characters() == 1 && to.characters() == 1) {
                // Range between from and to
                // Workaroud, cannot access entries in word other than with iterator
                char cfrom, cto;
                for(const auto& e: from) {
                    cfrom = e->getTerminal()->getChar();
                }
                for(const auto& e: to) {
                    cto = e->getTerminal()->getChar();
                }
                if (cfrom > cto) {
                    // Empty set
                    std::shared_ptr<Words::RegularConstraints::RegEmpty> empty(new Words::RegularConstraints::RegEmpty);
                    if (root == nullptr) {
                        root = empty;
                    } else {
                        parent->addChild(empty);
                    }
                } else {
                    // Union of all character between from and to
                    std::shared_ptr<Words::RegularConstraints::RegOperation> reunion(new Words::RegularConstraints::RegOperation(Words::RegularConstraints::RegularOperator::UNION));
                    for(auto f = cfrom; f <= cto; f++) {
                        // Add to context if not present
                        try {
                            ctxt.findSymbol(f);
                        }
                        catch (Words::WordException &e) {
                            ctxt.addTerminal(f);
                        }
                        Word chr;
                        auto builder = ctxt.makeWordBuilder(chr);
                        *builder << f;
                        builder->flush();
                        std::shared_ptr<Words::RegularConstraints::RegWord> rword(new Words::RegularConstraints::RegWord(chr));
                        reunion->addChild(rword);                
                    }
                    if (root == nullptr) {
                        root = reunion;
                    } else {
                        parent->addChild(reunion);
                    }
                }
            } else {
                // Empty set by defintion
                std::shared_ptr<Words::RegularConstraints::RegEmpty> empty(new Words::RegularConstraints::RegEmpty);
                if (root == nullptr) {
                    root = empty;
                } else {
                    parent->addChild(empty);
                }
                
            }
            

            
        }

        virtual void caseReNone(ReNone&) {
            std::shared_ptr<Words::RegularConstraints::RegEmpty> empty(new Words::RegularConstraints::RegEmpty);
            if (root == nullptr) {
                root = empty;
            } else {
                parent->addChild(empty);
            }
        }


        virtual void caseFunctionApplication(FunctionApplication&) {
            throw UnsupportedFeature();
        }

        virtual void caseEQ(EQ&) {/* Do nothing */}
	
    virtual void caseStrPrefixOf(StrPrefixOf&){}
	virtual void caseStrSuffixOf(StrSuffixOf &){}
	virtual void caseStrContains(StrContains &){}
    
        void run(ASTNode &node) {
            node.accept(*this);
        }

    private:

        void processRegOp(DummyApplication &c, Words::RegularConstraints::RegularOperator op) {
            std::shared_ptr<Words::RegularConstraints::RegOperation> node(new Words::RegularConstraints::RegOperation(op));

            if (root == nullptr) {
                // Top level operation, set as root and parent for next level
                root = node;
            } else {
                parent->addChild(node);
            }

            for (auto &cc: c) {
                parent = node;
                cc->accept(*this);
            }
        }

        Words::Context &ctxt;
        std::unique_ptr<WordBuilder> wordBuilder;
        std::shared_ptr<Words::RegularConstraints::RegNode> root = nullptr; // Points to the root of the regex tree
        std::shared_ptr<Words::RegularConstraints::RegOperation> parent = nullptr; // Points to the current parent during construction
        std::vector<Words::RegularConstraints::RegConstraint>&  reconstraints;


    };

    class MyErrorHandler : public SMTParser::ErrorHandler {
    public:
        MyErrorHandler(std::ostream &err) : os(err) {
        }

        virtual void error(const std::string &err) {
            herror = true;
            os << err << std::endl;
        }

        virtual bool hasError() { return herror; }

    private:
        std::ostream &os;
        bool herror = false;
    };

    class MyParser : public ParserInterface {
    public:

        MyParser(std::istream &is) : str(is) {}

        virtual std::unique_ptr<Words::JobGenerator> Parse(std::ostream &err) {
            auto jg = std::make_unique<JGenerator>();
            jg->context = std::make_shared<Words::Context>();
            MyErrorHandler handler(err);
            jg->parser.Parse(str, handler);
            std::stringstream str;
            for (auto &s: jg->parser.getVars()) {
                if (s->getSort() == Sort::String) {
                    str << *s << std::endl;
                    std::string ss = str.str();
                    ss.pop_back();
                    jg->context->addVariable(ss);
                    str.str("");
                }
            }

            TerminalAdder tadder(*jg->context);

            LengthConstraintBuilder lbuilder(*jg->context, jg->solver, jg->constraints, jg->eqs, jg->hashToLit,
                                             jg->neqmap);

            std::vector<Words::RegularConstraints::RegConstraint> rcs{};

            


            for (auto &t: jg->parser.getAssert()) {
                RegularConstraintBuilder rBuilder(*jg->context, rcs);
                t->accept(tadder);
                lbuilder.Run(*t);
                rBuilder.run(*t);

            }


            //std::shared_ptr<Words::RegularConstraints::RegNode> ex = rcs[0].expr;
        


            jg->recons = rcs;

            return jg;
        }

    private:
        std::istream &str;
    };

    std::unique_ptr<ParserInterface> makeSMTParser(std::istream &is) {
        return std::make_unique<MyParser>(is);
    }
}
