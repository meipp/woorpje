#import "regencoding.h"

using namespace Words::RegularConstraints;
using namespace Words;
using namespace std;

namespace RegularEncoding {

    namespace Automaton {

        int NFA::new_state() {
            return nQ++;
        }

        void NFA::set_initial_state(int q0) {
            initState = q0;
        }

        void NFA::add_final_state(int qf) {
            if (qf < 0 || qf >= nQ) {
                throw runtime_error("Unknown state");
            }
            final_states.insert(qf);
        }




        map<int, vector<pair<Terminal*, int>>> NFA::offset_states(int of) {
            map<int, vector<pair<Terminal*, int>>> newDelta{};
            for (auto trans: delta) {
                int q_src = trans.first + of;
                for (auto target: trans.second) {
                    Terminal* label = target.first;
                    int q_target = target.second + of;
                    if (newDelta.count(q_src) == 1) {
                        newDelta[q_src].push_back(make_pair(label, q_target));
                    } else {
                        vector<pair<Terminal*, int>> trans{make_pair(label, q_target)};
                        newDelta[q_src] = trans;
                    }
                }
            }
            return newDelta;
        }

        void NFA::add_transition(int src_q, Terminal* label, int target_q) {
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
                set<pair<Terminal*, int>> trans{make_pair(label, target_q)};
                delta[src_q] = trans;
            }
        }

        set<int> NFA::epsilonClosure(int q, set<int> ignoreStates) {
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
            for (int q=0; q < nQ; q++) {
                set<int> eclosure = epsilonClosure(q, set<int>{});
                for (int qe: eclosure) {
                    if (final_states.count(qe) == 1) {
                        add_final_state(q);
                    }
                    // Add transition q-a->q' if qe-a->q' exists
                    if (delta.count(qe) == 1) {
                        for(auto trans: delta[qe]) {
                            add_transition(q, trans.first, trans.second);
                        }
                    }
                }
            }
            // Remove all epsilon transitions
            map<int, set<pair<Terminal*, int>>> deltaWithoutEpsilon;
            for (const auto& trans: delta) {
                int q_src = trans.first;
                for (auto target: trans.second) {
                    int q_target = target.second;
                    Terminal* label = target.first;
                    if(!label->isEpsilon()) {
                        if (deltaWithoutEpsilon.count(q_src) == 1) {
                            deltaWithoutEpsilon[q_src].insert(make_pair(label, q_target));
                        } else {
                            set<pair<Terminal*, int>> tt{make_pair(label, q_target)};
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

            while(!queue.empty()) {
                int current = queue.front();
                queue.pop_front();
                if (getFinalStates().count(current) == 1) {
                    M.add_final_state(visited[current]);
                }
                if (delta.count(current) == 1) {
                    for (auto trans: delta[current]) {
                        if (visited.count(trans.second) == 0) {
                            visited[trans.second] = M.new_state();
                            queue.push_back(trans.second);
                        }
                        M.add_transition(visited[current], trans.first, visited[trans.second]);
                    }
                }
            }

            return M;


        }


        bool NFA::accept(const Words::Word& w) {
            set<int> s{initState};

            for (int q: epsilonClosure(initState, set<int>{})) {
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
                                for (auto epsq: epsilonClosure(trans.second, set<int>{})) {
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
                for (pair<Terminal*, int> target: trans.second) {
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


        NFA regexToNfa(Words::RegularConstraints::RegNode &regExpr, Words::Context& ctx) {
            try {
                RegWord &word = dynamic_cast<RegWord &>(regExpr);

                NFA M;
                int q0 = M.new_state();
                M.set_initial_state(q0);
                int q_src = q0;
                for (auto c: word.word) {
                    int q_target = M.new_state();
                    M.add_transition(q_src,  c->getTerminal(), q_target);

                    q_src = q_target;
                }


                M.add_final_state(q_src);
                return M;

            } catch (bad_cast &) {}

            try {
                RegOperation &opr = dynamic_cast<RegOperation &>(regExpr);
                switch (opr.getOperator()) {
                    case RegularOperator::CONCAT: {
                        // Concatenation
                        NFA M;
                        int q0 = M.new_state();
                        M.set_initial_state(q0);
                        set<int> qFs{q0};
                        for (auto sub: opr.getChildren()) {
                            NFA subM = regexToNfa(*sub, ctx);

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
                            for (auto trans: offsetDelta) {
                                for (auto target: trans.second) {
                                    M.add_transition(trans.first, target.first, target.second);
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
                            qFs = newqFs;
                        }
                        int qf = M.new_state();
                        M.add_final_state(qf);
                        for (int q: qFs) {
                            M.add_transition(q, ctx.getEpsilon(), qf);
                        }
                        return M;
                        break;
                    }
                    case RegularOperator::STAR: {

                        // Star
                        NFA M;
                        int q0 = M.new_state();
                        M.set_initial_state(q0);

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
                        for (auto trans: offsetDelta) {
                            for (auto target: trans.second) {
                                M.add_transition(trans.first, target.first, target.second);
                                if (subM.getFinalStates().count(target.second - off) == 1) {
                                    oldFs.insert(target.second);
                                }
                                if (subM.getInitialState() == trans.first - off) {

                                    oldQ0 = trans.first;
                                }
                            }
                        }

                        M.add_transition(q0, ctx.getEpsilon(), oldQ0);


                        int qf = M.new_state();



                        M.add_final_state(qf);
                        for (int q: oldFs) {
                            M.add_transition(q, ctx.getEpsilon(), qf);
                            M.add_transition(q, ctx.getEpsilon(), oldQ0);
                        }
                        M.add_transition(q0, ctx.getEpsilon(), qf);
                        return M;
                        break;
                    }

                    case RegularOperator::UNION: {
                        // Union
                        NFA M;
                        int q0 = M.new_state();
                        M.set_initial_state(q0);
                        list<int> oldQfs{};
                        list<int> oldq0s{};

                        for (auto sub: opr.getChildren()) {
                            NFA subM = regexToNfa(*sub, ctx);
                            int off = M.numStates();
                            auto offsetDelta = subM.offset_states(off);

                            for (int i = 0; i < subM.numStates(); i++) {
                                // Add numStates
                                M.new_state();
                            }
                            // Apply offset delta
                            for (auto trans: offsetDelta) {
                                for (auto target: trans.second) {
                                    M.add_transition(trans.first, target.first, target.second);
                                }
                            }

                            oldq0s.push_back(subM.getInitialState()+off);
                            for (int oldf: subM.getFinalStates()) {
                                oldQfs.push_back(oldf+off);
                            }
                        }



                        int qF = M.new_state();
                        M.add_final_state(qF);
                        for (int oqf: oldQfs) {
                            M.add_transition(oqf, ctx.getEpsilon(), qF);
                        }
                        for (int oq0: oldq0s) {
                            M.add_transition(q0, ctx.getEpsilon(), oq0);
                        }
                        return M;
                        break;
                    }
                }
            } catch (bad_cast &) {}

            try {
                RegEmpty &empty = dynamic_cast<RegEmpty &>(regExpr);
                NFA M;
                M.set_initial_state(M.new_state());
                return M;
            } catch (bad_cast &) {}
        }
    }
}