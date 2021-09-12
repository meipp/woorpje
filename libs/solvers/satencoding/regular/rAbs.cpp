
#include <algorithm>
#include <unordered_map>
#include "regencoding.h"
#include "commons.h"
#include <chrono>
#include <boost/math/common_factor.hpp>

using namespace std::chrono;


using namespace Words::RegularConstraints;
using namespace std;


namespace RegularEncoding {
    namespace LengthAbstraction {

        unordered_map<string, ArithmeticProgressions> cache{};

        namespace {
            ArithmeticProgressions concat(ArithmeticProgressions &lhs, ArithmeticProgressions &rhs) {
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

                        while (d * m < pow(b, 2) && d != 0) {
                            m++;
                        }
                        int nu = d * m;

                        ArithmeticProgressions P;
                        ArithmeticProgressions Q;
                        P.add(make_pair(a1 + a2 + nu, d));

                        int k1 = 0, k2 = 0;
                        while (b1 * k1 + b2 * k2 < pow(b, 2)) {
                            while (b1 * k1 + b2 * k2 <= pow(b, 2)) {
                                Q.add(make_pair(a1 + a2 + b1 * k1 + b2 * k2, 0));
                                k2++;
                                if (b2 * k2 == 0) {
                                    // Already found all k2
                                    break;
                                }
                            }
                            k2 = 0;
                            k1++;
                            if (b1 * k1 == 0) {
                                // Already found all k1
                                break;
                            }
                        }

                        Q.mergeOther(P);
                        result.mergeOther(Q);
                    }
                }
                return result;
            }

            ArithmeticProgressions star_single(pair<int, int> ab) {
                // Single element
                ArithmeticProgressions ap;
                if (ab.second == 0) {
                    ap.add(make_pair(0, ab.first));
                } else {
                    int lcm = boost::math::lcm(ab.first, ab.second);
                    for (int i = 1; i < lcm; i++) {
                        ap.add(make_pair(ab.first * i, boost::math::gcd(ab.first, ab.second)));
                    }

                }
                return ap;
            }

        }

        ArithmeticProgressions fromExpression(RegNode &expression) {

            stringstream ss;
            expression.toString(ss);
            string restr = ss.str();


            if (cache.count(restr) > 0) {
                return cache[restr];
            }
            try {
                RegWord &word = dynamic_cast<Words::RegularConstraints::RegWord &>(expression);
                // Single word w, return {(|w|, 0)}
                if (!word.word.noVariableWord()) {
                    throw new logic_error("Regular expression can't have variables");
                }
                ArithmeticProgressions ap;
                ap.add(make_pair(word.word.characters(), 0));
                cache[restr] = ap;

                return ap;
            } catch (std::bad_cast &) {}

            try {
                RegOperation &opr = dynamic_cast<Words::RegularConstraints::RegOperation &>(expression);
                switch (opr.getOperator()) {
                    case RegularOperator::CONCAT: {
                        // Concatenation
                        RegNode &lhs = *opr.getChildren()[0];
                        RegNode &rhs = *opr.getChildren()[1];
                        auto left = fromExpression(lhs);
                        auto right = fromExpression(rhs);
                        left = concat(left, right);
                        for (int i = 2; i < opr.getChildren().size(); i++) {
                            right = fromExpression(*opr.getChildren()[i]);
                            left = concat(left, right);
                        }
                        cache[restr] = left;
                        return left;
                        break;
                    }
                    case RegularOperator::STAR: {
                        ArithmeticProgressions sub = fromExpression(*opr.getChildren()[0]);
                        if (sub.getProgressions().size() == 1) {

                            pair<int, int> ab = *(sub.getProgressions().begin());
                            cache[restr] = star_single(ab);
                            return cache[restr];
                        } else {
                            vector<pair<int, int>> asvec(sub.getProgressions().begin(), sub.getProgressions().end());

                            auto ps = commons::powerset(asvec);

                            ArithmeticProgressions aps;
                            for (const vector<pair<int, int>> &subset: ps) {
                                for (pair<int, int> p: subset) {
                                    if (aps.getProgressions().empty()) {
                                        aps = star_single(p);
                                    } else {
                                        ArithmeticProgressions next = star_single(p);
                                        aps = concat(aps, next);
                                    }
                                }
                            }
                            cache[restr] = aps;
                            return aps;
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
            } catch (std::bad_cast &) {}

            try {
                RegEmpty &emps = dynamic_cast<Words::RegularConstraints::RegEmpty &>(expression);
                return {};
            } catch (std::bad_cast &) {}

        };


        /*
         * UNFALengthAbstractionBuilder
         */


        UNFALengthAbstractionBuilder::UNFALengthAbstractionBuilder(Automaton::NFA nfa) : nfa(nfa) {
            adjm = buildAdjacencyMatrix();
            if (!nfa.getDelta().empty()) {
                N = adjm.size();
                sccs = commons::scc(adjm);


                T = vector<shared_ptr<set<int>>>((int) pow(N, 2) - N - 1 + 1);
                set<int> F(nfa.getFinalStates());
                T[0] = make_unique<set<int>>(F);

                for (int i = 1; i < pow(N, 2) - N - 1 + 1; i++) {
                    T[i] = pre(T[i - 1]);
                }
            }
        }


        ArithmeticProgressions UNFALengthAbstractionBuilder::forState(int q) {

            if (nfa.getDelta().empty()) {
                if (nfa.getFinalStates().count(nfa.getInitialState()) > 0) {
                    ArithmeticProgressions aps;
                    aps.add(make_pair(0,0));
                    return aps;
                } else {
                    return {};
                }

            }

            vector<shared_ptr<set<int>>> S((int) pow(N, 2));

            S[0] = make_unique<set<int>>(set<int>{q});


            auto start = chrono::high_resolution_clock::now();
            for (int i = 1; i < pow(N, 2); i++) {
                S[i] = succ(S[i - 1]);
                //cout << "|S[" << i << "]| = " << S[i]->size() << "\n";
            }
            auto duration = chrono::duration_cast<milliseconds>(chrono::high_resolution_clock::now() - start);

            //cout << "S in " << duration.count() << "ms" << endl;


            set<int> imp;
            map<int, int> sls;

            for (set<int> scc: sccs) {

                if (scc.size() == 1) {
                    sls[*scc.begin()] = 0;
                } else {
                    int minSlScc = -1;
                    set<int> ignored;
                    for (int v = 0; v < N; v++) {
                        if (scc.count(v) == 0) {
                            // v is not in SCC, ignore it
                            ignored.insert(v);
                        }
                    }

                    for (int v: scc) {
                        sls[v] = sl(v, ignored);
                        if (sls[v] < minSlScc || minSlScc < 0) {
                            minSlScc = sls[v];
                        }
                    }

                    // Add to set of important states imp if mininal in SCC
                    for (int v: scc) {
                        if (sls[v] == minSlScc) {
                            imp.insert(v);
                        }
                    }

                }
            }

            // Define Qimp: sl(q) -> Q
            map<int, set<int>> qImp;
            for (int v: *S[N - 1]) {
                if (imp.count(v) > 0) {
                    if (qImp.count(sls[v]) > 0) {
                        qImp[sls[v]].insert(v);
                    } else {
                        qImp[sls[v]] = set<int>{v};
                    }
                }
            }


            ArithmeticProgressions aps;

            // R1
            for (int i = 0; i < pow(N, 2); i++) {
                for (int v: *S[i]) {
                    if (T[0]->count(v) > 0) {
                        aps.add(make_pair(i, 0));
                    }
                }
            }


            for (int c = (int) (pow(N, 2)) - 2 * N; c < (int) (pow(N, 2)) - N; c++) {
                for (auto impstate: qImp) {
                    int d = impstate.first;
                    set<int> qs = impstate.second;
                    if (c >= (int) (pow(N, 2)) - N - d && c <= (int) (pow(N, 2)) - N) {
                        for (int v: qs) {
                            if (T[c]->count(v) > 0) {
                                int cc = (N - 1) + c;
                                aps.add(make_pair(cc, d));
                                break;
                            }
                        }
                    }
                }
            }


            return aps;
        }

        /**
         * Finds all successors for all states in set S.
         * @param S a set of states
         * @return a set containing all successors to states in S
         */
        std::shared_ptr<std::set<int>> UNFALengthAbstractionBuilder::succ(std::shared_ptr<std::set<int>> &S) {

            if (successorsCache.count(*S) == 1) {
                //cout << "HIT!\n";
                return successorsCache[*S];
            }
            set<int> Sn;
            for (int s: *S) {
                for (int r = 0; r < N; r++) {
                    if (adjm[s][r]) {
                        Sn.insert(r);
                    }
                }
            }

            auto res = make_shared<set<int>>(Sn);
            successorsCache[*S] = res;
            return res;
        }

        /**
         * Finds all predecessor for all states in set S.
         * @param P a set of states
         * @return a set containing all predecessors to states in P
         */
        shared_ptr<set<int>> UNFALengthAbstractionBuilder::pre(std::shared_ptr<std::set<int>> &P) {


            set<int> Pn;
            for (int p: *P) {
                for (int r = 0; r < N; r++) {
                    if (adjm[r][p]) {
                        Pn.insert(r);
                    }
                }
            }
            return make_shared<set<int>>(Pn);
        }

        int UNFALengthAbstractionBuilder::sl(int q, set<int> ignore) {
            list<int> queue;
            set<int> visited;
            map<int, int> lengths;
            for (int v: ignore) {
                visited.insert(v);
            }
            for (int v: visited) {
                lengths[v] = -1;
            }


            int sl = -1;
            // Add all adjacent nodes with path length one (direct successors)
            for (int v = 0; v < N; v++) {
                if (adjm[q][v]) {
                    queue.push_back(v);
                    lengths[v] = 1;
                    visited.insert(v);
                }
            }
            // Perform BFS searching for q, updating path lengths
            while (!queue.empty()) {
                int current = queue.front();
                queue.pop_front();
                int currentPathLength = lengths[current];
                if (current == q) {
                    // Found path, check if path is shorter
                    if (sl < 0 || currentPathLength < sl) {
                        sl = currentPathLength;
                    }
                }
                for (int v = 0; v < N; v++) {
                    if (adjm[current][v]) {
                        if (visited.count(v) == 0) {
                            queue.push_back(v);
                            visited.insert(v);
                            lengths[v] = currentPathLength + 1;
                        } else {
                            // Update length if current path is shorter
                            if (lengths[v] > currentPathLength + 1) {
                                queue.push_back(v); // Update all paths from v
                                lengths[v] = currentPathLength + 1;
                            }
                        }
                    }
                }
            }

            if (sl < 0) {
                throw runtime_error("NFA is not connected");
            }

            return sl;
        }

        vector<vector<bool>> UNFALengthAbstractionBuilder::buildAdjacencyMatrix() {
            int n = nfa.numStates();
            vector<vector<bool>> matrix(n);
            for (int i = 0; i < n; i++) {
                vector<bool> row(n, false);
                matrix[i] = row;
            }

            for (auto trans: nfa.getDelta()) {
                int qsrc = trans.first;
                for (auto target: trans.second) {
                    int qt = target.second;
                    matrix[qsrc][qt] = true;
                }
            }
            return matrix;

        }


    }
}