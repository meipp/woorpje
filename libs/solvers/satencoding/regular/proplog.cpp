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
        void PLFormula::make_binary() {
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
                subformulae[i].make_binary();
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
            tuple<PLFormula, int>
            defstep(Junctor, PLFormula&, PLFormula&, vector<tuple<int, PLFormula>>&, int, Glucose::Solver& solver);

            tuple<PLFormula, int>
            maincnf(PLFormula& f, vector<tuple<int, PLFormula>>& defs, int n, Glucose::Solver& solver) {

                if (f.getJunctor() == Junctor::AND) {

                    return defstep(Junctor::AND, f.getSubformulae()[0], f.getSubformulae()[1], defs, n, solver);
                } else if (f.getJunctor() == Junctor::OR) {
                    if (f.getSubformulae().size() != 2) {
                        cout << "Error: OR must have two operands but has " << f.getSubformulae().size()  << "\n";
                        cout << "Formula: " << f.getSubformulae()[0].toString();
                        exit(-1);
                    }
                    return defstep(Junctor::OR, f.getSubformulae()[0], f.getSubformulae()[1], defs, n, solver);
                } else if (f.getJunctor() == Junctor::NOT) {
                    int lit = -f.getSubformulae()[0].getLiteral();
                    PLFormula ff = PLFormula::lit(lit);
                    return make_tuple(ff,  n);
                } else {
                    // Literal
                    return make_tuple(f,  n);
                }

            }

            tuple<PLFormula, int>
            defstep(Junctor junctor, PLFormula& f1, PLFormula& f2, vector<tuple<int, PLFormula>>& defs, int n, Glucose::Solver& solver) {


                tuple<PLFormula, int> left = maincnf(f1, defs, n, solver);
                tuple<PLFormula,  int> right = maincnf(f2, defs, n, solver);
                // get<1>(left)

                int n2 = solver.newVar();
                PLFormula phi = (junctor == Junctor::AND) ? PLFormula::land(
                        vector<PLFormula>{get<0>(left), get<0>(right)})
                                                          : PLFormula::lor(
                                vector<PLFormula>{get<0>(left), get<0>(right)});
                tuple<int, PLFormula> newDef = make_tuple(n2, phi);
                defs.push_back(newDef);
                return make_tuple(PLFormula::lit(n2), n2 + 1);
            }
        }

        set<set<int>> tseytin_cnf(PLFormula& formula, Glucose::Solver &solver) {

            /*if (!formula.is_nenf()) {
                throw invalid_argument("Formula must be in NENF");
            }*/

            int max_var = formula.max_var();

            formula.make_binary();

            cout << "\tmaincnf...";
            cout.flush();
            vector<tuple<int, PLFormula>> tmp{};
            tuple<PLFormula, int> tseytin_conf = maincnf(formula,
                                                                                        tmp,
                                                                                        max_var + 1, solver);
            cout << "ok\n";
            //cout << "Got tseytin form\n";
            PLFormula phi = get<0>(tseytin_conf);
            //vector<tuple<int, PLFormula>> defs = get<1>(tseytin_conf);


            set<set<int>> cnf{};
            cnf.insert(set<int>{phi.getLiteral()});



            for (int i = 0; i < tmp.size(); i++) {

                int l = get<0>(tmp[i]);
                PLFormula fl = get<1>(tmp[i]);


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