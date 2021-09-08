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
                return subformulae.size() == 0;
            } else if (junctor == Junctor::AND || junctor == Junctor::OR) {
                return subformulae.size() >= 2;
            } else if (junctor == Junctor::NOT) {
                return subformulae.size() == 1;
            }


            return false;

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

        void PLFormula::make_binary() {
            if (junctor == Junctor::AND || junctor == Junctor::OR) {
                if (subformulae.size() > 2) {
                    PLFormula left = subformulae[0];
                    vector<PLFormula> rightsubs(subformulae.begin() + 1, subformulae.end());
                    PLFormula right = PLFormula(junctor, rightsubs);
                    subformulae = vector<PLFormula>{left, right};
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
            tuple<PLFormula, vector<tuple<int, PLFormula>>, int>
            defstep(Junctor, PLFormula, PLFormula, vector<tuple<int, PLFormula>>, int);

            tuple<PLFormula, vector<tuple<int, PLFormula>>, int>
            maincnf(PLFormula f, vector<tuple<int, PLFormula>> defs, int n) {

                if (f.getJunctor() == Junctor::AND) {
                    return defstep(Junctor::AND, f.getSubformulae()[0], f.getSubformulae()[1], defs, n);
                } else if (f.getJunctor() == Junctor::OR) {
                    return defstep(Junctor::OR, f.getSubformulae()[0], f.getSubformulae()[1], defs, n);
                } else if (f.getJunctor() == Junctor::NOT) {
                    int lit = -f.getSubformulae()[0].getLiteral();
                    PLFormula ff = PLFormula::lit(lit);
                    return make_tuple(ff, defs, n);
                } else {
                    // Literal
                    return make_tuple(f, defs, n);
                }
            }

            tuple<PLFormula, vector<tuple<int, PLFormula>>, int>
            defstep(Junctor junctor, PLFormula f1, PLFormula f2, vector<tuple<int, PLFormula>> defs, int n) {

                tuple<PLFormula, vector<tuple<int, PLFormula>>, int> left = maincnf(f1, defs, n);
                tuple<PLFormula, vector<tuple<int, PLFormula>>, int> right = maincnf(f2, get<1>(left), get<2>(left));

                int n2 = get<2>(right);
                PLFormula phi = (junctor == Junctor::AND) ? PLFormula::land(
                        vector<PLFormula>{get<0>(left), get<0>(right)}) : PLFormula::lor(
                        vector<PLFormula>{get<0>(left), get<0>(right)});
                tuple<int, PLFormula> newDef = make_tuple(n2, phi);

                vector<tuple<int, PLFormula>> newdefs = get<1>(right);
                newdefs.push_back(newDef);

                return make_tuple(PLFormula::lit(n2), newdefs, n2 + 1);
            }
        }

        set<set<int>> tseytin_cnf(PLFormula formula) {
            if (!formula.is_nenf()) {
                throw invalid_argument("Formula must be in NENF");
            }

            int max_var = formula.max_var();
            formula.make_binary();


            tuple<PLFormula, vector<tuple<int, PLFormula>>, int> tseytin_conf = maincnf(formula,
                                                                                        vector<tuple<int, PLFormula>>{},
                                                                                        max_var + 1);
            PLFormula phi = get<0>(tseytin_conf);
            vector<tuple<int, PLFormula>> defs = get<1>(tseytin_conf);


            set<set<int>> cnf{};
            cnf.insert(set<int>{phi.getLiteral()});


            for (int i = 0; i < defs.size(); i++) {
                int l = get<0>(defs[i]);
                PLFormula fl = get<1>(defs[i]);

                cout << l << "\n";

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