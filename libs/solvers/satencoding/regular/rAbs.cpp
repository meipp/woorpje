
#include <algorithm>
#include <unordered_map>
#include <map>
#include "regencoding.h"
#include "commons.h"
#include <chrono>
#include <omp.h>
#include <boost/math/common_factor.hpp>

using namespace chrono;


using namespace Words::RegularConstraints;
using namespace std;


namespace RegularEncoding {
    namespace LengthAbstraction {

        unordered_map<string, shared_ptr<ArithmeticProgressions>> cache{};
        map<pair<string, int>, shared_ptr<ArithmeticProgressions>> nfaCache{};
        unordered_map<string, vector<shared_ptr<set<int>>>> nfaTcache{};

        namespace {
            ArithmeticProgressions concat(ArithmeticProgressions &lhs, ArithmeticProgressions &rhs) {
                ArithmeticProgressions result;
                for (pair<int, int> ab1: lhs.getProgressions()) {
                    int a1 = ab1.first;
                    int b1 = ab1.second;
                    for (pair<int, int> ab2: rhs.getProgressions()) {


                        int a2 = ab2.first;
                        int b2 = ab2.second;

                        if (b1 == 0 && b2 == 0) {
                            result.add(make_pair(a1 + a2, 0));
                            break;
                        }

                        int d = __gcd(b1, b2);

                        int nu = ((b1 - d) * (b2 - d)) / d + a1 + a2;

                        ArithmeticProgressions P;
                        ArithmeticProgressions Q;
                        P.add(make_pair(nu, d));

                        int k1 = 0, k2 = 0;
                        while (b1 * k1 + b2 * k2 < nu - (2 * d)) {
                            while (b1 * k1 + b2 * k2 <= nu - (2 * d)) {
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
                return *cache[restr];
            }
            try {
                RegWord &word = dynamic_cast<Words::RegularConstraints::RegWord &>(expression);
                // Single word w, return {(|w|, 0)}
                ArithmeticProgressions ap;
                ap.add(make_pair(word.word.characters(), 0));
                cache[restr] = make_shared<ArithmeticProgressions>(ap);

                return ap;
            } catch (bad_cast &) {}

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
                        cache[restr] = make_shared<ArithmeticProgressions>(left);
                        return left;
                        break;
                    }
                    case RegularOperator::STAR: {
                        ArithmeticProgressions sub = fromExpression(*opr.getChildren()[0]);
                        ArithmeticProgressions aps;

                        vector<int> allconsts;
                        int n = (int) sub.getProgressions().size();
                        allconsts.reserve(n * 2);
                        for (auto ab: sub.getProgressions()) {
                            allconsts.push_back(ab.first);
                            allconsts.push_back(ab.second);
                        }
                        int d = boost::integer::gcd_range(allconsts.begin(), allconsts.end()).first;
                        int c = boost::integer::lcm_range(allconsts.begin(), allconsts.end()).first;
                        //ArithmeticProgressions Q;
                        aps.add(make_pair(c, d));
                        for (pair<int, int> ab: sub.getProgressions()) {
                            int a = ab.first;
                            int b = ab.second;
                            int k1 = 0, k2 = 0;
                            while (a * k1 + b * k2 < c) {
                                while (a * k1 + b * k2 <= c) {
                                    aps.add(make_pair(a*k1 + b*k2, 0));
                                    k2++;
                                    if (b * k2 == 0) {
                                        // Already found all k2
                                        break;
                                    }
                                }
                                k2 = 0;
                                k1++;
                                if (a * k1 == 0) {
                                    // Already found all k1
                                    break;
                                }
                            }
                        }
                        
                        cache[restr] = make_shared<ArithmeticProgressions>(aps);
                        return aps;

                    }
                    case RegularOperator::UNION: {
                        // Union
                        ArithmeticProgressions first = fromExpression(*opr.getChildren()[0]);
                        for (int i = 1; i < opr.getChildren().size(); i++) {
                            ArithmeticProgressions next = fromExpression(*opr.getChildren()[i]);
                            first.mergeOther(next);
                        }
                        cache[restr] = make_shared<ArithmeticProgressions>(first);

                        return first;
                    }
                }
            } catch (bad_cast &) {}

            try {
                RegEmpty &emps = dynamic_cast<Words::RegularConstraints::RegEmpty &>(expression);
                return {};
            } catch (bad_cast &) {}

        };


        /*
         * UNFALengthAbstractionBuilder
         */

        // i = transtitions done, N = max transtitions
        vector<vector<set<int>>> buildS(int N, map<int, set<pair<Words::Terminal *, int>>> &delta) {
            auto start = chrono::high_resolution_clock::now();

            vector<vector<set<int>>> Sq(N);
            omp_set_num_threads(omp_get_num_procs());
            for (int q = 0; q < N; q++) {
                Sq[q] = vector<set<int>>((int) pow(N, 2));
                Sq[q][0] = set<int>{q};
                for (int i = 1; i < pow(N, 2); i++) {
                    Sq[q][i] = set<int>{};
                }
            }

            int p = pow(N, 2);

            omp_set_num_threads(omp_get_num_procs());
            for (int i = 1; i < p; i++) {
#pragma omp parallel for schedule(dynamic)
                for (int q = 0; q < N; q++) {
                    for (int pre: Sq[q][i - 1]) {
                        for (auto &tr: delta[pre]) {
                            Sq[q][i].insert(tr.second);
                            //cout << "inserted " << tr.second << "\n";
                        }
                    }
                }
            }

            auto stop = chrono::high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(stop - start);
            cout << "####### buildS took " << duration.count() << "ms\n";
            cout.flush();
            return Sq;
        }

        UNFALengthAbstractionBuilder::UNFALengthAbstractionBuilder(Automaton::NFA nfa) : nfa(nfa) {
            adjm = buildAdjacencyMatrix();
            statewiserAbs = std::map<int, std::shared_ptr<ArithmeticProgressions>>{};
            N = int(adjm.size());
            auto delta = nfa.getDelta();
            Sq = buildS(N, delta);

            if (!nfa.getDelta().empty()) {

                sccs = commons::scc(adjm);

                if (nfaTcache.count(nfa.toString()) > 0) {
                    T = nfaTcache[nfa.toString()];
                } else {
                    T = vector<shared_ptr<set<int>>>((int) pow(N, 2) - N - 1 + 1);
                    set<int> F(nfa.getFinalStates());
                    T[0] = make_unique<set<int>>(F);

                    for (int i = 1; i < pow(N, 2) - N - 1 + 1; i++) {
                        T[i] = pre(T[i - 1]);
                    }
                    nfaTcache[nfa.toString()] = T;


                }
            } else {

                for (int q = 0; q < N; q++) {
                    ArithmeticProgressions empty;
                    if (nfa.getFinalStates().count(q) > 0) {
                        empty.add(make_pair(0, 0));
                    }
                    cout << "Setting " << q << endl;
                    statewiserAbs[q] = make_shared<ArithmeticProgressions>(empty);
                }
            }

        }


        ArithmeticProgressions UNFALengthAbstractionBuilder::forStateComplete(int q) {




            // No transitions, length abstraction is (0,0) if q0 in F, and {} otherwise
            if (nfa.getDelta().empty()) {
                if (nfa.getFinalStates().count(nfa.getInitialState()) > 0) {
                    ArithmeticProgressions aps;
                    aps.add(make_pair(0, 0));
                    return aps;
                } else {
                    return {};
                }

            }


            vector<set<int>> &S = Sq[q];


            set<int> imp;
            map<int, int> sls;

            for (const set<int> &scc: sccs) {

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
            for (int v: S[N - 1]) {
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
                for (int v: S[i]) {
                    if (T[0]->count(v) > 0) {
                        aps.add(make_pair(i, 0));
                    }
                }
            }


            for (int c = (int) (pow(N, 2)) - 2 * N; c < (int) (pow(N, 2)) - N; c++) {
                for (const auto &impstate: qImp) {
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

            const string nfastr = nfa.toString();
            nfaCache[make_pair(nfastr, q)] = make_shared<ArithmeticProgressions>(aps);
            statewiserAbs[q] = make_shared<ArithmeticProgressions>(aps);

            return aps;
        }


        ArithmeticProgressions UNFALengthAbstractionBuilder::forState(int q) {

            // Check predecessors;
            const string nfastr = nfa.toString();
            //cout << "hier\n";
            if (nfaCache.count(make_pair(nfastr, q)) > 0) {

                //cout<< "other\n";
                return *nfaCache[make_pair(nfastr, q)];
            }
            if (statewiserAbs.count(q) > 0) {
                auto aps = *statewiserAbs[q];
                nfaCache[make_pair(nfastr, q)] = make_shared<ArithmeticProgressions>(aps);
                cout << "statewise\n";
                return aps;
            }


            bool haspred = false;
            ArithmeticProgressions aps;
            //cout << "Hier für " << q << "\n";
            for (int v = 0; v < N; v++) {
                if (adjm[v][q]) {
                    haspred = true;
                    // Same scc?
                    bool sameScc = false;
                    for (auto &scc: sccs) {
                        if (scc.count(v) == 1 && scc.count(q) == 1) {
                            sameScc = true;
                            break;
                        }
                    }
                    ArithmeticProgressions apsV;
                    if (statewiserAbs.count(v)) {
                        apsV = *statewiserAbs[v];
                    } else if (nfaCache.count(make_pair(nfastr, v)) > 0) {
                        apsV = *nfaCache[make_pair(nfastr, v)];
                    } else {
                        if (!sameScc) {
                            apsV = forState(v);
                        } else {
                            // v and q in same scc, prevent infite recursion
                            //cout << q << "==" << v << endl;
                            apsV = forStateComplete(v);
                            //cout << "OK\n";
                            auto shr = make_shared<ArithmeticProgressions>(apsV);
                            nfaCache[make_pair(nfastr, v)] = shr;
                            statewiserAbs[v] = shr;

                        }
                    }

                    for (pair<int, int> ap: apsV.getProgressions()) {
                        //cout << "Building...\n";
                        if (sameScc && ap.second > 0) {

                            aps.add(make_pair((ap.first - 1) % ap.second, ap.second));

                        } else {
                            if ((ap.first - 1) >= 0) {
                                aps.add(make_pair((ap.first - 1), ap.second));
                            }
                        }
                    }
                }
            }

            if (!haspred) {
                aps = forStateComplete(q);
            } else {
                //cout << "Return für " << q << "\n";
                statewiserAbs[q] = make_shared<ArithmeticProgressions>(aps);
            }
            nfaCache[make_pair(nfastr, q)] = make_shared<ArithmeticProgressions>(aps);
            return aps;

        }

        /**
         * Finds all successors for all states in set S.
         * @param S a set of states
         * @return a set containing all successors to states in S
         */
        shared_ptr<set<int>> UNFALengthAbstractionBuilder::succ(shared_ptr<set<int>> &S) {

            if (successorsCache.count(*S) == 1) {
                return successorsCache[*S];
            }
            set<int> Sn;
            for (int s: *S) { // O(n)
                for (int r = 0; r < N; r++) { // O(n)
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
        shared_ptr<set<int>> UNFALengthAbstractionBuilder::pre(shared_ptr<set<int>> &P) {

            if (predecessorCache.count(*P) == 1) {
                return predecessorCache[*P];
            }
            set<int> Pn;
            for (int p: *P) {
                for (int r = 0; r < N; r++) {
                    if (adjm[r][p]) {
                        Pn.insert(r);
                    }
                }
            }
            auto res = make_shared<set<int>>(Pn);
            predecessorCache[*P] = res;
            return res;
        }

        int UNFALengthAbstractionBuilder::sl(int q, const set<int> &ignore) {
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