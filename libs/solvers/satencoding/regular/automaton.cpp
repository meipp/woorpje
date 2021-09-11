#import "regencoding.h"

using namespace Words::RegularConstraints;
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
                throw new runtime_error("Unknown state");
            }
            final_states.insert(qf);
        }

        map<int, vector<pair<string, int>>> NFA::offset_states(int of) {
            map<int, vector<pair<string, int>>> newDelta{};
            for (auto trans: delta) {
                int q_src = trans.first + of;
                for (auto target: trans.second) {
                    string label = target.first;
                    int q_target = target.second + of;
                    if (newDelta.count(q_src) == 1) {
                        newDelta[q_src].push_back(make_pair(label, q_target));
                    } else {
                        vector<pair<string, int>> trans{make_pair(label, q_target)};
                        newDelta[q_src] = trans;
                    }
                }
            }
            return newDelta;
        }

        void NFA::add_transition(int src_q, string label, int target_q) {
            if (src_q < 0 || src_q >= nQ) {
                throw new runtime_error("Invalid source state");
            }
            if (target_q < 0 || target_q >= nQ) {
                throw new runtime_error("Invalid target state");
            }
            if (label.size() > 1) {
                throw new runtime_error("Invalid label, must be empty or one character");
            }
            if (delta.count(src_q) == 1) {
                delta[src_q].insert(make_pair(label, target_q));
            } else {
                set<pair<string, int>> trans{make_pair(label, target_q)};
                delta[src_q] = trans;
            }
        }

        set<int> NFA::epsilonClosure(int q) {
            set<int> closure{};
            if (delta.count(q) == 0) {
                return closure;
            }
            for (auto trans: delta[q]) {
                if (trans.first.empty()) {
                    closure.insert(trans.second);
                    set<int> rec = epsilonClosure(trans.second);
                    closure.insert(rec.begin(), rec.end());
                }
            }
            return closure;
        }

        void NFA::removeEpsilonTransitions() {
            // Add direct transitions, skipping e-transitions
            for (int q=0; q < nQ; q++) {
                set<int> eclosure = epsilonClosure(q);
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
            map<int, set<pair<string, int>>> deltaWithoutEpsilon;
            for (auto trans: delta) {
                int q_src = trans.first;
                for (auto target: trans.second) {
                    int q_target = target.second;
                    string label = target.first;
                    if(!label.empty()) {
                        if (deltaWithoutEpsilon.count(q_src) == 1) {
                            deltaWithoutEpsilon[q_src].insert(make_pair(label, q_target));
                        } else {
                            set<pair<string, int>> trans{make_pair(label, q_target)};
                            deltaWithoutEpsilon[q_src] = trans;
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
                for (pair<string, int> target: trans.second) {
                    ss << "(" << target.first << ", " << target.second << ")";
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


        NFA regex_to_nfa(Words::RegularConstraints::RegNode &regExpr) {
            try {
                RegWord &word = dynamic_cast<RegWord &>(regExpr);

                NFA M;
                int q0 = M.new_state();
                M.set_initial_state(q0);
                int q_src = q0;
                for (auto c: word.word) {
                    int q_target = M.new_state();
                    M.add_transition(q_src, string(1, c->getTerminal()->getChar()), q_target);

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
                            NFA subM = regex_to_nfa(*sub);
                            int off = M.states();
                            auto offsetDelta = subM.offset_states(off);
                            for (int i = 0; i < subM.states(); i++) {
                                // Add states
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
                                            M.add_transition(qf, "", trans.first);
                                        }
                                    }
                                }
                            }
                            qFs = newqFs;
                        }
                        int qf = M.new_state();
                        M.add_final_state(qf);
                        for (int q: qFs) {
                            M.add_transition(q, "", qf);
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
                        int oldQ0;
                        auto sub = opr.getChildren()[0];
                        NFA subM = regex_to_nfa(*sub);
                        int off = M.states();
                        auto offsetDelta = subM.offset_states(off);
                        for (int i = 0; i < subM.states(); i++) {
                            // Add states
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

                        M.add_transition(q0, "", oldQ0);
                        int qf = M.new_state();
                        M.add_transition(q0, "", qf);
                        M.add_final_state(qf);
                        for (int q: oldFs) {
                            M.add_transition(q, "", qf);
                            M.add_transition(q, "", oldQ0);
                        }
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
                            NFA subM = regex_to_nfa(*sub);
                            int off = M.states();
                            auto offsetDelta = subM.offset_states(off);

                            for (int i = 0; i < subM.states(); i++) {
                                // Add states
                                M.new_state();
                            }
                            // Apply offset delta
                            for (auto trans: offsetDelta) {
                                for (auto target: trans.second) {
                                    M.add_transition(trans.first, target.first, target.second);
                                    if (subM.getFinalStates().count(target.second - off) == 1) {
                                        oldQfs.push_back(target.second);
                                    }
                                    if (subM.getInitialState() == trans.first - off) {
                                        oldq0s.push_back(trans.first);
                                    }
                                }
                            }
                        }
                        int qF = M.new_state();
                        M.add_final_state(qF);
                        for (int oqf: oldQfs) {
                            M.add_transition(oqf, "", qF);
                        }
                        for (int oq0: oldq0s) {
                            M.add_transition(q0, "", oq0);
                        }
                        return M;
                        break;
                    }
                }
            } catch (bad_cast &) {}

            try {
                RegEmpty &empty = dynamic_cast<RegEmpty &>(regExpr);
                // Return automaton with single state that does not accept anything
                NFA M;
                M.set_initial_state(M.new_state());
                return M;
            } catch (bad_cast &) {}
        }
    }
}