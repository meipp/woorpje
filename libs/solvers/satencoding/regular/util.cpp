#include <set>
#include <unordered_set>
#include <vector>

#include "encoding.h"
#include "regencoding.h"

using namespace std;
using namespace Words::RegularConstraints;

namespace RegularEncoding {

int numTerminals(std::vector<FilledPos> &pat, int from, int to) {
    if (to < 0) {
        to = pat.size();
    }
    int ts = 0;
    for (int i = from; i < to; i++) {
        if (pat[i].isTerminal()) {
            ts++;
        }
    }
    return ts;
}

/**
 * Creates a filled pattern out of a word (containing variables).
 * Each variable x is substituted by maxPadding(x) pairs.
 * @param pattern the pattern
 * @return a vector containing the indices of the filled pattern
 */
vector<FilledPos> Encoder::filledPattern(const Words::Word &pattern) {
    vector<FilledPos> filledPattern{};

    for (auto s : pattern) {
        if (s->isTerminal()) {
            int ci = tIndices->at(s->getTerminal());
            FilledPos tidx(ci);
            filledPattern.push_back(tidx);
        } else if (s->isVariable()) {
            int xi = vIndices->at(s->getVariable());
            for (int j = 0; j < maxPadding->at(xi); j++) {
                FilledPos vidx(make_pair(xi, j));
                filledPattern.push_back(vidx);
            }
        }
    }

    return filledPattern;
}

}  // namespace RegularEncoding