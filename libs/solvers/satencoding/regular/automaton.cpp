#import "regencoding.h"

using namespace Words::RegularConstraints;
using namespace Words;
using namespace std;

namespace RegularEncoding::Automaton {

        int NFA::new_state() {
            int s = nQ++;
            this->minReachable[s] = INT_MAX;
            this->maxReachable[s] = -1;
            return s;
        }

        void NFA::set_initial_state(int q0) {
            minReachable[q0] = 0;
            initState = q0;
        }

        void NFA::add_final_state(int qf) {
            if (qf < 0 || qf >= nQ) {
                throw runtime_error("Unknown state");
            }
            final_states.insert(qf);
        }


        map<int, vector<pair<Terminal *, int>>> NFA::offset_states(int of) {
            map<int, vector<pair<Terminal *, int>>> newDelta{};
            for (const auto &trans: delta) {
                int q_src = trans.first + of;
                for (auto target: trans.second) {
                    Terminal *label = target.first;
                    int q_target = target.second + of;
                    if (newDelta.count(q_src) == 1) {
                        newDelta[q_src].push_back(make_pair(label, q_target));
                    } else {
                        vector<pair<Terminal *, int>> newTrans{make_pair(label, q_target)};
                        newDelta[q_src] = newTrans;
                    }
                }
            }
            return newDelta;
        }

        void NFA::add_transition(int src_q, Terminal *label, int target_q) {
            if (src_q < 0 || src_q >= nQ) {
                throw runtime_error("Invalid source state");
            }
            if (target_q < 0 || target_q >= nQ) {
                stringstream ss;
                ss << "Invalid target state " << target_q << ", nQ is " << nQ << endl;
                throw runtime_error(ss.str());
            }
            if (delta.count(src_q) == 1) {
                delta[src_q].insert(make_pair(label, target_q));
            } else {
                set<pair<Terminal *, int>> trans{make_pair(label, target_q)};
                delta[src_q] = trans;
            }
        }

        set<int> NFA::epsilonClosure(int q, set<int> &ignoreStates) {
            set<int> closure{};
            if (delta.count(q) == 0) {

                return closure;

            }
            // Don't follow self transitions
            ignoreStates.insert(q);
            for (auto trans: delta[q]) {
                if (trans.first->isEpsilon() && ignoreStates.count(trans.second) == 0) {
                    closure.insert(trans.second);
                    ignoreStates.insert(trans.second);
                    set<int> rec = epsilonClosure(trans.second, ignoreStates);
                    closure.insert(rec.begin(), rec.end());
                }
            }
            return closure;
        }

        void NFA::removeEpsilonTransitions() {
            if (delta.empty()) {
                return;
            }
            // Add direct transitions, skipping e-transitions
            for (int q = 0; q < nQ; q++) {

                auto tmp = set<int>{};
                set<int> eclosure = epsilonClosure(q, tmp);
                for (int qe: eclosure) {
                    if (final_states.count(qe) == 1) {
                        add_final_state(q);
                    }
                    // Add transition q-a->q' if qe-a->q' exists
                    if (delta.count(qe) == 1) {
                        for (auto trans: delta[qe]) {
                            add_transition(q, trans.first, trans.second);
                        }
                    }
                }
            }
            // Remove all epsilon transitions
            map<int, set<pair<Terminal *, int>>> deltaWithoutEpsilon;
            for (const auto &trans: delta) {
                int q_src = trans.first;
                for (auto target: trans.second) {
                    int q_target = target.second;
                    Terminal *label = target.first;
                    if (!label->isEpsilon()) {
                        if (deltaWithoutEpsilon.count(q_src) == 1) {
                            deltaWithoutEpsilon[q_src].insert(make_pair(label, q_target));
                        } else {
                            set<pair<Terminal *, int>> tt{make_pair(label, q_target)};
                            deltaWithoutEpsilon[q_src] = tt;
                        }
                    }
                }
            }
            delta = deltaWithoutEpsilon;
        }

        NFA NFA::reduceToReachableState() {

            NFA M;
            // Always reachable
            int q0 = M.new_state();
            M.set_initial_state(q0);

            list<int> queue{};
            map<int, int> visited;
            visited[getInitialState()] = q0;

            queue.push_back(getInitialState());
            M.minReachable[q0] = this->minReachable[getInitialState()];
            M.maxReachable[q0] = this->maxReachable[getInitialState()];

            while (!queue.empty()) {
                int current = queue.front();
                queue.pop_front();
                if (getFinalStates().count(current) == 1) {
                    M.add_final_state(visited[current]);
                }
                if (delta.count(current) == 1) {
                    for (auto trans: delta[current]) {
                        if (visited.count(trans.second) == 0) {
                            int qn = M.new_state();
                            visited[trans.second] = qn;
                            M.minReachable[qn] = this->minReachable[trans.second];
                            M.maxReachable[qn] = this->maxReachable[trans.second];
                            queue.push_back(trans.second);
                        }
                        M.add_transition(visited[current], trans.first, visited[trans.second]);

                    }
                }
            }

            return M;


        }


        bool NFA::accept(const Words::Word &w) {
            set<int> s{initState};
            auto tmp = set<int>{};
            for (int q: epsilonClosure(initState, tmp)) {
                s.insert(q);
            }

            for (auto e: w) {
                if (e->isVariable()) {
                    return false;
                }
                set<int> succs;
                for (int q: s) {
                    if (delta.count(q) > 0) {
                        for (auto trans: delta[q]) {
                            if (trans.first->getChar() == (e->getTerminal()->getChar()) || trans.first->isEpsilon()) {
                                succs.insert(trans.second);
                                auto tmp1 = set<int>{};
                                for (auto epsq: epsilonClosure(trans.second, tmp1)) {
                                    succs.insert(epsq);
                                }
                            }
                        }
                    }
                }
                s = succs;
            }


            for (int qf: getFinalStates()) {
                if (s.count(qf) > 0) {
                    return true;
                }
            }
            return false;
        }

        string NFA::toString() {
            stringstream ss;
            ss << "Q = {";
            for (int q = 0; q < nQ - 1; q++) {
                ss << q << ", ";
            }
            ss << nQ - 1 << "};  ";
            ss << "delta = {";
            for (auto trans: delta) {
                ss << trans.first << ": [";
                for (pair<Terminal *, int> target: trans.second) {
                    ss << "(" << target.first->getChar() << ", " << target.second << ")";
                }
                ss << "], ";
            }
            ss << "}  ";

            ss << "q0 = " << initState << ";  ";
            ss << "F = {";

            for (auto qf: final_states) {
                ss << qf << ", ";
            }
            ss << "};  ";

            return ss.str();

        }


        NFA regexToNfa(Words::RegularConstraints::RegNode &regExpr, Words::Context &ctx) {
            try {
                // WORD
                auto &word = dynamic_cast<RegWord &>(regExpr);
                NFA M;
                int q0 = M.new_state();
                M.set_initial_state(q0);
                int numTrans = 0;
                M.minReachable[q0] = numTrans;
                M.maxReachable[q0] = numTrans;
                numTrans++;
                int q_src = q0;
                for (auto c: word.word) {
                    int q_target = M.new_state();
                    M.minReachable[q_target] = numTrans;
                    M.maxReachable[q_target] = numTrans;
                    numTrans++;
                    M.add_transition(q_src, c->getTerminal(), q_target);
                    q_src = q_target;
                }
                M.add_final_state(q_src);

                for (int s = 0; s < M.numStates(); s++) {
                    assert(M.minReachable[s] > -1);
                    assert(M.maxReachable[s] > -1 );
                }
                return M;

            } catch (bad_cast &) {}

            try {
                auto &opr = dynamic_cast<RegOperation &>(regExpr);
                switch (opr.getOperator()) {
                    case RegularOperator::CONCAT: {
                        // Concatenation
                        NFA M;
                        int q0 = M.new_state();
                        M.set_initial_state(q0);
                        M.minReachable[q0] = 0;
                        M.maxReachable[q0] = 0;
                        set<int> qFs{q0};
                        for (const auto &sub: opr.getChildren()) {
                            NFA subM = regexToNfa(*sub, ctx);

                            for (int s = 0; s < subM.numStates(); s++) {
                                assert(subM.maxReachable[s] > -1);
                            }

                            if (subM.getDelta().empty()) {
                                continue;
                            }

                            int off = M.numStates();
                            auto offsetDelta = subM.offset_states(off);
                            for (int i = 0; i < subM.numStates(); i++) {
                                // Add numStates
                                M.new_state();
                            }
                            // Apply offset delta
                            set<int> newqFs{};
                            for (const auto &trans: offsetDelta) {
                                M.minReachable[trans.first] = subM.minReachable[trans.first - off];
                                M.maxReachable[trans.first] = subM.maxReachable[trans.first - off];
                                assert(M.maxReachable[trans.first] > -1);
                                for (auto target: trans.second) {
                                    M.add_transition(trans.first, target.first, target.second);
                                    // Keep reachability information
                                    M.minReachable[target.second] = subM.minReachable[target.second - off];
                                    M.maxReachable[target.second] = subM.maxReachable[target.second - off];
                                    assert(M.maxReachable[target.second] > -1);
                                    if (subM.getFinalStates().count(target.second - off) == 1) {
                                        newqFs.insert(target.second);
                                    }
                                    if (subM.getInitialState() == trans.first - off) {
                                        for (int qf: qFs) {
                                            M.add_transition(qf, ctx.getEpsilon(), trans.first);
                                        }
                                    }
                                }
                            }
                            int maxMaxReachable = -1;
                            int minMinReachable = INT_MAX;
                            for (int q: qFs) {
                                maxMaxReachable = std::max(M.maxReachable[q], maxMaxReachable);
                                minMinReachable = std::min(M.minReachable[q], minMinReachable);
                            }
                            assert(minMinReachable > -1);
                            assert(maxMaxReachable > -1);
                            for (int s = 0; s < subM.numStates(); s++) {
                                M.minReachable[s + off] = subM.minReachable[s] + minMinReachable;
                                if (subM.maxReachable[s] == INT_MAX || maxMaxReachable == INT_MAX) {
                                    M.maxReachable[s + off] = INT_MAX;
                                } else {
                                    M.maxReachable[s + off] = subM.maxReachable[s] + maxMaxReachable;
                                }

                                assert(M.maxReachable[s + off] > -1);
                            }
                            qFs = newqFs;
                        }
                        int qf = M.new_state();
                        M.add_final_state(qf);

                        int maxMaxReachable = -1;
                        int minMinReachable = INT_MAX;
                        for (int q: qFs) {
                            minMinReachable = std::min(minMinReachable, M.minReachable[q]);
                            maxMaxReachable = std::max(maxMaxReachable, M.maxReachable[q]);
                            M.add_transition(q, ctx.getEpsilon(), qf);
                        }
                        assert(minMinReachable > -1);
                        assert(maxMaxReachable > -1);
                        M.minReachable[qf] = minMinReachable;
                        M.maxReachable[qf] = maxMaxReachable;


                        for (int s = 0; s < M.numStates(); s++) {
                            assert(M.minReachable[s] > -1);
                            assert(M.maxReachable[s] > -1);
                        }

                        return M;
                        break;
                    }
                    case RegularOperator::STAR: {

                        // Star
                        NFA M;
                        int q0 = M.new_state();
                        M.set_initial_state(q0);
                        M.minReachable[q0] = 0;
                        M.maxReachable[q0] = 0;

                        set<int> oldFs{};
                        int oldQ0 = 0;
                        auto sub = opr.getChildren()[0];
                        NFA subM = regexToNfa(*sub, ctx);

                        int off = M.numStates();
                        auto offsetDelta = subM.offset_states(off);
                        for (int i = 0; i < subM.numStates(); i++) {
                            // Add numStates
                            M.new_state();
                        }
                        // Apply offset delta
                        for (const auto &trans: offsetDelta) {
                            M.minReachable[trans.first] = subM.minReachable[trans.first - off];
                            M.maxReachable[trans.first] = subM.maxReachable[trans.first - off];
                            for (auto target: trans.second) {
                                M.add_transition(trans.first, target.first, target.second);
                                M.minReachable[target.second] = subM.minReachable[target.second - off];
                                M.maxReachable[target.second] = subM.maxReachable[target.second - off];
                                if (subM.getFinalStates().count(target.second - off) == 1) {
                                    oldFs.insert(target.second);
                                }
                                if (subM.getInitialState() == trans.first - off) {

                                    oldQ0 = trans.first;
                                }
                            }
                        }
                        for (int s = 0; s < subM.numStates(); s++) {
                            M.maxReachable[s + off] = INT_MAX;
                        }

                        M.add_transition(q0, ctx.getEpsilon(), oldQ0);

                        int qf = M.new_state();

                        M.add_final_state(qf);
                        for (int q: oldFs) {
                            //M.add_transition(q, ctx.getEpsilon(), qf);
                            //M.add_transition(q, ctx.getEpsilon(), oldQ0);
                            M.add_transition(q, ctx.getEpsilon(), q0);
                        }
                        M.maxReachable[q0] = INT_MAX;
                        M.add_transition(q0, ctx.getEpsilon(), qf);
                        M.maxReachable[qf] = M.maxReachable[q0];
                        M.minReachable[qf] = M.minReachable[q0];

                        for (int s = 0; s < M.numStates(); s++) {
                            assert(M.minReachable[s] > -1);
                            assert(M.maxReachable[s] > -1);
                        }
                        return M;
                        break;
                    }

                    case RegularOperator::UNION: {
                        // Union
                        NFA M;
                        int q0 = M.new_state();
                        M.minReachable[q0] = 0;
                        M.maxReachable[q0] = 0;
                        M.set_initial_state(q0);
                        list<int> oldQfs{};
                        list<int> oldq0s{};
                        for (const auto &sub: opr.getChildren()) {
                            NFA subM = regexToNfa(*sub, ctx);
                            int off = M.numStates();
                            auto offsetDelta = subM.offset_states(off);

                            for (int i = 0; i < subM.numStates(); i++) {
                                // Add numStates
                                M.new_state();
                            }
                            // Apply offset delta
                            for (const auto &trans: offsetDelta) {
                                M.minReachable[trans.first] = subM.minReachable[trans.first - off];
                                M.maxReachable[trans.first] = subM.maxReachable[trans.first - off];
                                assert(M.maxReachable[trans.first] > -1);
                                for (auto target: trans.second) {
                                    M.minReachable[target.second] = subM.minReachable[target.second - off];
                                    M.maxReachable[target.second] = subM.maxReachable[target.second - off];
                                    assert(M.maxReachable[target.second] > -1);
                                    M.add_transition(trans.first, target.first, target.second);
                                }
                            }

                            oldq0s.push_back(subM.getInitialState() + off);
                            // Special case when subM has only one state an no transition ("")
                            M.minReachable[subM.getInitialState() + off] = subM.minReachable[subM.getInitialState()];
                            M.maxReachable[subM.getInitialState() + off] = subM.maxReachable[subM.getInitialState()];
                            for (int oldf: subM.getFinalStates()) {
                                oldQfs.push_back(oldf + off);
                            }

                        }


                        int qF = M.new_state();
                        M.add_final_state(qF);
                        for (int oqf: oldQfs) {
                            M.add_transition(oqf, ctx.getEpsilon(), qF);
                            M.minReachable[qF] = std::min(M.minReachable[qF], M.minReachable[oqf]);
                            M.maxReachable[qF] = std::max(M.maxReachable[qF], M.maxReachable[oqf]);
                            assert(M.maxReachable[qF] > -1);
                        }
                        for (int oq0: oldq0s) {
                            M.add_transition(q0, ctx.getEpsilon(), oq0);
                        }
                        for (int s = 0; s < M.numStates(); s++) {
                            // assert(M.minReachable[s] <= M.numStates());
                            assert(M.maxReachable[s] > -1);
                        }
                        return M;
                        break;
                    }
                }
            } catch (bad_cast &) {}

            try {
                auto &empty = dynamic_cast<RegEmpty &>(regExpr);
                NFA M;
                auto s = M.new_state();
                M.set_initial_state(s);
                M.minReachable[s] = 0;
                M.maxReachable[s] = 0;
                return M;
            } catch (bad_cast &) {}
        }
    }