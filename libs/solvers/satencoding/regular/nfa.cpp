#include "regencoding.h"

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

map<int, vector<pair<Terminal *, int>>> NFA::offset_delta(int of) {
    map<int, vector<pair<Terminal *, int>>> newDelta{};
    for (const auto &trans : delta) {
        int q_src = trans.first + of;
        for (auto target : trans.second) {
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

map<int, set<int>> NFA::offset_delta_epsilon(int of) {
    map<int, set<int>> newDelta{};
    for (const auto &trans : deltaEpsilon) {
        int q_src = trans.first + of;
        for (auto target : trans.second) {
            int q_target = target + of;
            if (newDelta.count(q_src) == 1) {
                newDelta[q_src].insert(q_target);
            } else {
                set<int> newTrans{q_target};
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

void NFA::add_eps_transition(int src_q, int target_q) {
    if (src_q < 0 || src_q >= nQ) {
        throw runtime_error("Invalid source state");
    }
    if (target_q < 0 || target_q >= nQ) {
        stringstream ss;
        ss << "Invalid target state " << target_q << ", nQ is " << nQ << endl;
        throw runtime_error(ss.str());
    }
    if (deltaEpsilon.count(src_q) == 1) {
        deltaEpsilon[src_q].insert(target_q);
    } else {
        set<int> etrans{target_q};
        deltaEpsilon[src_q] = etrans;
    }
}

set<int> NFA::epsilonClosure(int q, set<int> &ignoreStates) {
    set<int> closure{};
    if (delta.count(q) == 0 && deltaEpsilon.count(q) == 0) {
        // Unknown state
        return closure;
    }
    // Don't follow self transitions
    ignoreStates.insert(q);
    if(deltaEpsilon.count(q) > 0) {
        for (auto qtarget : deltaEpsilon[q]) {
            if (ignoreStates.count(qtarget) == 0) {
                closure.insert(qtarget);
                ignoreStates.insert(qtarget);
                set<int> rec = epsilonClosure(qtarget, ignoreStates);
                closure.insert(rec.begin(), rec.end());
            }
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
        for (int qe : eclosure) {
            if (final_states.count(qe) == 1) {
                add_final_state(q);
            }
            // Add transition q-a->q' if qe-a->q' exists
            if (delta.count(qe) == 1) {
                for (auto trans : delta[qe]) {
                    add_transition(q, trans.first, trans.second);
                }
            }
        }
    }
    // Remove all epsilon transitions
    deltaEpsilon.clear();
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
    M.maxrAbs[q0] = this->maxrAbs[getInitialState()];
    M.minrAbs[q0] = this->minrAbs[getInitialState()];

    while (!queue.empty()) {
        int current = queue.front();
        queue.pop_front();
        if (getFinalStates().count(current) == 1) {
            M.add_final_state(visited[current]);
        }
        if (delta.count(current) == 1) {
            for (auto trans : delta[current]) {
                if (visited.count(trans.second) == 0) {
                    int qn = M.new_state();
                    visited[trans.second] = qn;
                    M.minReachable[qn] = this->minReachable[trans.second];
                    M.maxReachable[qn] = this->maxReachable[trans.second];
                    M.maxrAbs[qn] = this->maxrAbs[trans.second];
                    M.minrAbs[qn] = this->minrAbs[trans.second];
                    queue.push_back(trans.second);
                }
                M.add_transition(visited[current], trans.first, visited[trans.second]);
            }
        }
        if (deltaEpsilon.count(current) == 1) {
            for (auto trans : deltaEpsilon[current]) {
                if (visited.count(trans) == 0) {
                    int qn = M.new_state();
                    visited[trans] = qn;
                    M.minReachable[qn] = this->minReachable[trans];
                    M.maxReachable[qn] = this->maxReachable[trans];
                    M.maxrAbs[qn] = this->maxrAbs[trans];
                    M.minrAbs[qn] = this->minrAbs[trans];
                    queue.push_back(trans);
                }
                M.add_eps_transition(visited[current], visited[trans]);
            }
        }
    }
    return M;
}

bool NFA::accept(const Words::Word &w) {
    set<int> s{initState};
    auto tmp = set<int>{};
    for (int q : epsilonClosure(initState, tmp)) {
        s.insert(q);
    }

    for (auto e : w) {
        if (e->isVariable()) {
            return false;
        }
        set<int> succs;
        for (int q : s) {
            if (delta.count(q) > 0) {
                for (auto trans : delta[q]) {
                    if (trans.first->getChar() == (e->getTerminal()->getChar()) || trans.first->isEpsilon()) {
                        succs.insert(trans.second);
                        auto tmp1 = set<int>{};
                        for (auto epsq : epsilonClosure(trans.second, tmp1)) {
                            succs.insert(epsq);
                        }
                    }
                }
            }
        }
        s = succs;
    }

    for (int qf : getFinalStates()) {
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
    for (auto trans : delta) {
        ss << trans.first << ": [";
        for (pair<Terminal *, int> target : trans.second) {
            ss << "(" << target.first->getChar() << ", " << target.second << ")";
        }
        ss << "], ";
    }
    ss << "}  ";
    ss << "epsilons = {";
    for (auto &trans : deltaEpsilon) {
        ss << trans.first << ": [";
        for (int dest : trans.second) {
            ss << dest << " ";
        }
        ss << "], ";
    }
    ss << "}  ";

    ss << "q0 = " << initState << ";  ";
    ss << "F = {";

    for (auto qf : final_states) {
        ss << qf << ", ";
    }
    ss << "};  ";

    return ss.str();
}

// TODO: Refactor this
NFA regexToNfa(Words::RegularConstraints::RegNode &regExpr, Words::Context &ctx) {
    try {
        // WORD
        auto &word = dynamic_cast<RegWord &>(regExpr);
        NFA M;
        int N = word.characters();
        int q0 = M.new_state();
        M.set_initial_state(q0);
        int numTrans = 0;
        M.minReachable[q0] = numTrans;
        M.maxReachable[q0] = numTrans;
        M.maxrAbs[q0] = N;
        M.minrAbs[q0] = N;
        numTrans++;
        int q_src = q0;
        for (auto c : word.word) {
            int q_target = M.new_state();
            M.minReachable[q_target] = numTrans;
            M.maxReachable[q_target] = numTrans;
            M.maxrAbs[q_target] = N - numTrans;
            M.minrAbs[q_target] = N - numTrans;
            numTrans++;
            M.add_transition(q_src, c->getTerminal(), q_target);
            q_src = q_target;
        }
        M.add_final_state(q_src);
        assert(M.maxrAbs[q0] == M.minrAbs[q0] && M.minrAbs[q0] == N);
        return M;
    } catch (bad_cast &) {
    }

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
                for (const auto &sub : opr.getChildren()) {
                    NFA subM = regexToNfa(*sub, ctx);
                    if (subM.getDelta().empty()) {
                        continue;
                    }

                    int off = M.numStates();

                    for (int i = 0; i < subM.numStates(); i++) {
                        // Add numStates
                        M.new_state();
                    }

                    set<int> newqFs{};

                    // Apply offset delta
                    auto offsetDelta = subM.offset_delta(off);
                    for (const auto &trans : offsetDelta) {
                        for (auto &target : trans.second) {
                            M.add_transition(trans.first, target.first, target.second);
                        }
                    }
                    auto offesetDeltaEps = subM.offset_delta_epsilon(off);
                    for (const auto &epsTrans : offesetDeltaEps) {
                        for (const auto &target : epsTrans.second) {
                            M.add_eps_transition(epsTrans.first, target);
                        }
                    }

                    // Update initial/final states
                    for (int i = 0; i < subM.numStates(); i++) {
                        int q = i + off;
                        if (subM.getInitialState() == i) {
                            for (int qf : qFs) {
                                // From last final states to current initial state
                                M.add_eps_transition(qf, q);
                            }
                        }
                        if (subM.getFinalStates().count(i) > 0) {
                            // Save new final statess
                            newqFs.insert(q);
                        }
                        M.minrAbs[q] = subM.minrAbs[i];
                        M.maxrAbs[q] = subM.maxrAbs[i];
                    }

                    // Update reachability and rAbs information

                    // Find min/max reachble in final current states
                    int maxMaxReachable = -1;
                    int minMinReachable = INT_MAX;
                    for (int q : qFs) {
                        maxMaxReachable = std::max(M.maxReachable[q], maxMaxReachable);
                        minMinReachable = std::min(M.minReachable[q], minMinReachable);
                    }

                    int maxMaxrAbs = -1;
                    int minMinrAbs = INT_MAX;
                    for (int s = 0; s < subM.numStates(); s++) {
                        M.minReachable[s + off] = subM.minReachable[s] + minMinReachable;
                        if (subM.maxReachable[s] == INT_MAX || maxMaxReachable == INT_MAX) {
                            M.maxReachable[s + off] = INT_MAX;
                        } else {
                            M.maxReachable[s + off] = subM.maxReachable[s] + maxMaxReachable;
                        }
                        maxMaxrAbs = std::max(maxMaxrAbs, subM.maxrAbs[s]);
                        minMinrAbs = std::min(minMinrAbs, subM.minrAbs[s]);
                        assert(M.maxReachable[s + off] > -1);
                        
                    }
                    // Update old rabs
                    assert(maxMaxrAbs > 0);
                    for (int s = 0; s < off; s++) {
                        M.maxrAbs[s] = M.maxrAbs[s] + maxMaxrAbs;
                        if (M.maxrAbs[s] < 0) {
                            // Detect overflow
                            M.maxrAbs[s] = INT_MAX;
                        }
                        M.minrAbs[s] = M.minrAbs[s] + minMinrAbs;
                        
                    }
                    qFs = newqFs;
                }
                int qf = M.new_state();
                M.add_final_state(qf);

                int maxMaxReachable = -1;
                int minMinReachable = INT_MAX;
                for (int q : qFs) {
                    minMinReachable = std::min(minMinReachable, M.minReachable[q]);
                    maxMaxReachable = std::max(maxMaxReachable, M.maxReachable[q]);
                    M.add_eps_transition(q, qf);
                }
                M.minReachable[qf] = minMinReachable;
                M.maxReachable[qf] = maxMaxReachable;
                assert(minMinReachable > -1);
                assert(maxMaxReachable > -1);
                opr.toString(cout);
                M.minReachable[qf] = minMinReachable;
                M.maxReachable[qf] = maxMaxReachable;
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

                if (subM.getDelta().empty()) {
                    // Star over empty automaton is the automaton itself
                    // eps^* = eps, </>^* = </>
                    return subM;
                }

                int off = M.numStates();

                for (int i = 0; i < subM.numStates(); i++) {
                    // Add numStates
                    M.new_state();
                }
                auto offesetDeltaEps = subM.offset_delta_epsilon(off);
                for (const auto &epsTrans : offesetDeltaEps) {
                    for (const auto &target : epsTrans.second) {
                        M.add_eps_transition(epsTrans.first, target);
                    }
                }
                auto offsetDelta = subM.offset_delta(off);
                for (const auto &trans : offsetDelta) {
                    for (auto &target : trans.second) {
                        M.add_transition(trans.first, target.first, target.second);
                    }
                }
                // Update initial/final states
                for (int i = 0; i < subM.numStates(); i++) {
                    int q = i + off;
                    if (subM.getInitialState() == i) {
                         oldQ0 = q;
                    }
                    if (subM.getFinalStates().count(i) > 0) {
                        oldFs.insert(q);
                    }
                    // Keep reachabilty/rAbs information
                    M.minReachable[q] = subM.minReachable[i];
                    M.minrAbs[q] = subM.minrAbs[i];
                    M.maxReachable[q] = INT_MAX;
                    M.maxrAbs[q] = INT_MAX;
                }


                M.add_eps_transition(q0, oldQ0);
                int qf = M.new_state();
                M.add_final_state(qf);
                for (int q : oldFs) {
                    M.add_eps_transition(q, q0);
                }
                M.maxReachable[q0] = INT_MAX;
                M.maxrAbs[q0] = INT_MAX;
                M.add_eps_transition(q0, qf);
                M.maxReachable[qf] = M.maxReachable[q0];
                M.maxrAbs[qf] = M.maxrAbs[q0];
                M.minReachable[qf] = M.minReachable[q0];
                M.minrAbs[qf] = M.minrAbs[q0];

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
                for (const auto &sub : opr.getChildren()) {
                    NFA subM = regexToNfa(*sub, ctx);
                    int off = M.numStates();

                    for (int i = 0; i < subM.numStates(); i++) {
                        // Add numStates
                        int s = M.new_state();
                    }
                    auto offsetDelta = subM.offset_delta(off);
                    for (const auto &trans : offsetDelta) {
                        for (auto &target : trans.second) {
                            M.add_transition(trans.first, target.first, target.second);
                        }
                    }
                    auto offesetDeltaEps = subM.offset_delta_epsilon(off);
                    for (const auto &epsTrans : offesetDeltaEps) {
                        for (const auto &target : epsTrans.second) {
                            M.add_eps_transition(epsTrans.first, target);
                        }
                    }
                    // Update initial/final states
                    for (int i = 0; i < subM.numStates(); i++) {
                        int q = i + off;
                        if (subM.getInitialState() == i) {
                            M.add_eps_transition(q0, q);
                            M.maxrAbs[q0] = max(M.maxrAbs[q0], subM.maxrAbs[i]);
                            M.minrAbs[q0] = max(M.minrAbs[q0], subM.minrAbs[i]);
                        }
                        if (subM.getFinalStates().count(i) > 0) {
                            oldQfs.push_back(q);
                        }
                        // Keep reachabilty/rAbs information
                        M.minReachable[q] = subM.minReachable[i];
                        M.maxReachable[q] = subM.maxReachable[i];
                        M.minrAbs[q] = subM.minrAbs[i];
                        M.maxrAbs[q] = subM.maxrAbs[i];
                    }
                }
                // Create new final state
                int qF = M.new_state();
                M.add_final_state(qF);
                for (int oqf : oldQfs) {
                    M.add_eps_transition(oqf, qF);
                    M.minReachable[qF] = std::min(M.minReachable[qF], M.minReachable[oqf]);
                    M.maxReachable[qF] = std::max(M.maxReachable[qF], M.maxReachable[oqf]);
                    M.minrAbs[qF] = std::min(M.minrAbs[qF], M.minrAbs[oqf]);
                    M.maxrAbs[qF] = std::max(M.maxrAbs[qF], M.maxrAbs[oqf]);
                }
                return M;
                break;
            }
        }
    } catch (bad_cast &) {
    }

    try {
        auto &empty = dynamic_cast<RegEmpty &>(regExpr);
        NFA M;
        auto s = M.new_state();
        M.set_initial_state(s);
        M.minReachable[s] = 0;
        M.maxReachable[s] = 0;
        return M;
    } catch (bad_cast &) {
    }
}
}  // namespace RegularEncoding::Automaton