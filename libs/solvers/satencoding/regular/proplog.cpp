#include "regencoding.h"
#include <vector>
#include <set>
#include <iostream>

using namespace std;

namespace RegularEncoding {

    namespace PropositionalLogic {

        PLFormula::PLFormula(Junctor junctor, std::vector<PLFormula> subformulae) : junctor(junctor),
                                                                                    subformulae(subformulae),
                                                                                    literal(0) {}

        PLFormula::PLFormula(int literal) : junctor(Junctor::LIT), literal(literal), subformulae(vector<PLFormula>{}) {}

        bool PLFormula::valid() {
            if (junctor == Junctor::LIT) {
                if (subformulae.size() != 0) {
                    std::cout << "Literals can' have operands\n";
                    return false;
                }
            } else if (junctor == Junctor::AND || junctor == Junctor::OR) {
                if (subformulae.size() < 2) {
                    std::cout << "AND/OR must have at least 2 operands\n";
                    return false;
                }
            } else if (junctor == Junctor::NOT) {
                if (subformulae.size() != 1) {
                    std::cout << "NOT can only have one operand\n";
                    return false;
                }
            }

            for (auto s: subformulae) {
                return s.valid();
            }

            return true;

        }

        string PLFormula::toString() {
            stringstream ss;

            switch (junctor) {

                case Junctor::LIT:
                    ss << literal;
                    break;
                case Junctor::AND:
                    ss << "AND (";
                    for (auto s: subformulae) {
                        ss << s.toString();
                        ss << " ";
                    }
                    ss << ") ";
                    break;
                case Junctor::OR:
                    ss << "OR(";
                    for (auto s: subformulae) {
                        ss << s.toString();
                        ss << " ";
                    }
                    ss << ") ";
                    break;
                case Junctor::NOT:
                    ss << "-(";
                    for (auto s: subformulae) {
                        ss << s.toString();
                        ss << " ";
                    }
                    ss << ") ";
                    break;
            }

            return ss.str();
        }

        bool PLFormula::is_nenf() {
            if (junctor == Junctor::NOT) {
                if (subformulae[0].junctor != Junctor::LIT) {
                    return false;
                }
            } else if (junctor != Junctor::LIT) {
                for (int i = 0; i < subformulae.size(); i++) {
                    if (!subformulae[i].is_nenf()) {
                        return false;
                    }
                }
            }
            return true;
        }

        // TODO: Use pointers instread of copying
        void PLFormula::makeBinary() {
            if (junctor == Junctor::AND || junctor == Junctor::OR) {
                if (subformulae.size() > 2) {
                    PLFormula left = subformulae[0];
                    vector<PLFormula> rightsubs(subformulae.begin() + 1, subformulae.end());
                    PLFormula right = PLFormula(junctor, rightsubs);
                    subformulae = vector<PLFormula>{left, right};
                } else if (subformulae.size() == 1) {
                    auto subf = subformulae[0];
                    junctor = subf.junctor;
                    literal = subf.literal;
                    subformulae = subf.subformulae;
                } else if (subformulae.empty()) {
                    cout << "Invalid formula, junctor has " << subformulae.size() << " operands\n";
                    cout << toString() << "\n";
                    exit(-1);
                }
            }
            for (int i = 0; i < subformulae.size(); i++) {
                subformulae[i].makeBinary();
            }
        }

        int PLFormula::depth() {
            if (junctor == Junctor::LIT) {
                return 1;
            } else {
                int max = -1;
                for (int i = 0; i < subformulae.size(); i++) {
                    int d = subformulae[i].depth();
                    if (d > max) {
                        max = d;
                    }
                }
                return max + 1;
            }
        }

        int PLFormula::size() {
            if (junctor == Junctor::LIT) {
                return 1;
            } else {
                int sz = 1;
                for (int i = 0; i < subformulae.size(); i++) {
                    int d = subformulae[i].size();
                    sz+=d;
                }
                return sz;
            }
        }

        int PLFormula::max_var() {
            if (junctor == Junctor::LIT) {
                return abs(literal);
            } else {
                int max = -1;
                for (int i = 0; i < subformulae.size(); i++) {
                    int v = subformulae[i].max_var();
                    if (v > max) {
                        max = v;
                    }
                }
                return max;
            }
        }

        PLFormula PLFormula::lit(int literal) {
            return PLFormula(literal);
        }

        PLFormula PLFormula::land(std::vector<PLFormula> subformulae) {
            return PLFormula(Junctor::AND, subformulae);
        }

        PLFormula PLFormula::lor(std::vector<PLFormula> subformulae) {
            return PLFormula(Junctor::OR, subformulae);
        }

        PLFormula PLFormula::lnot(PLFormula subformula) {
            return PLFormula(Junctor::NOT, vector<PLFormula>{subformula});
        }


        namespace {
            unique_ptr<PLFormula>
            defstep(Junctor, PLFormula &, PLFormula &, list<tuple<int, PLFormula>> &, Glucose::Solver &solver);

            unique_ptr<PLFormula>
            maincnf(PLFormula &f, list<tuple<int, PLFormula>> &defs, Glucose::Solver &solver) {

                if (f.getJunctor() == Junctor::AND) {

                    return defstep(Junctor::AND, f.getSubformulae()[0], f.getSubformulae()[1], defs, solver);
                } else if (f.getJunctor() == Junctor::OR) {
                    return defstep(Junctor::OR, f.getSubformulae()[0], f.getSubformulae()[1], defs, solver);
                } else if (f.getJunctor() == Junctor::NOT) {
                    int lit = -f.getSubformulae()[0].getLiteral();
                    PLFormula ff = PLFormula::lit(lit);
                    return make_unique<PLFormula>(ff);
                } else {
                    // Literal
                    return make_unique<PLFormula>(f);
                }

            }

            unique_ptr<PLFormula>
            defstep(Junctor junctor, PLFormula &f1, PLFormula &f2, list<tuple<int, PLFormula>> &defs,
                    Glucose::Solver &solver) {


                unique_ptr<PLFormula> left = maincnf(f1, defs, solver);
                unique_ptr<PLFormula> right = maincnf(f2, defs, solver);

                int n2 = solver.newVar();
                PLFormula phi = (junctor == Junctor::AND) ? PLFormula::land(
                        vector<PLFormula>{*left, *right})
                                                          : PLFormula::lor(
                                vector<PLFormula>{*left, *right});
                tuple<int, PLFormula> newDef = make_tuple(n2, phi);
                defs.push_back(newDef);
                auto res = PLFormula::lit(n2);
                return make_unique<PLFormula>(res);
            }
        }

        set<set<int>> tseytin_cnf(PLFormula &formula, Glucose::Solver &solver) {

            formula.makeBinary();


            cout.flush();
            list<tuple<int, PLFormula>> tmp{};

            PLFormula phi = *maincnf(formula, tmp, solver);

            set<set<int>> cnf{};
            cnf.insert(set<int>{phi.getLiteral()});

            for (auto def: tmp) {

                int l = get<0>(def);
                PLFormula fl = get<1>(def);


                if (fl.getJunctor() == Junctor::AND) {
                    // l <-> fl1 /\ ... /\ fln <==> (-l \/ fl1) /\ ... /\ (-l \/ fln) /\ (-fl1 \/ ... \/ -fln \/ l)
                    set<int> rtl = set<int>{l};
                    for (int n = 0; n < fl.getSubformulae().size(); n++) {
                        PLFormula fln = fl.getSubformulae()[n];
                        rtl.insert(-fln.getLiteral());
                        set<int> lrtn{-l, fln.getLiteral()};
                        cnf.insert(lrtn);
                    }
                    cnf.insert(rtl);
                } else if (fl.getJunctor() == Junctor::OR) {
                    // l <-> fl1 \/ ... \/ fln <==> (-l \/ fl1 \/ ... \/ fln) /\ (l \/ -fl1) ... /\ (l \/ -fln)
                    set<int> ltr = set<int>{-l};
                    for (int n = 0; n < fl.getSubformulae().size(); n++) {
                        PLFormula fln = fl.getSubformulae()[n];
                        ltr.insert(fln.getLiteral());
                        set<int> rtln{l, -fln.getLiteral()};
                        cnf.insert(rtln);
                    }
                    cnf.insert(ltr);
                } else {
                    throw logic_error("Something went wrong :S");
                }

            }
            return cnf;
        }

    }
}