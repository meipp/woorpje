#include "catch2/catch.hpp"
#include <list>
#include <set>
#include <algorithm>
#include <iostream>

#include "../lib/commons.cpp"


using namespace std;
using namespace commons;


TEST_CASE("Powersets") {
    CHECK(powerset(vector<int>{1}) == vector<vector<int>>{vector<int>{}, vector<int>{1}});
    CHECK(powerset(vector<int>{1,2}) == vector<vector<int>>{vector<int>{}, vector<int>{1},vector<int>{2}, vector<int>{1, 2}});
    CHECK(powerset(vector<int>{1,2,3}) == vector<vector<int>>{vector<int>{}, vector<int>{1},vector<int>{2}, vector<int>{1, 2}, vector<int>{3}, vector<int>{1, 3}, vector<int>{2, 3}, vector<int>{1, 2, 3}});
}

TEST_CASE("Strongly Connected Components") {

    vector<vector<bool>> adj_matrix = {
        vector<bool>{false, true, false, false, false, false, false, false},
        vector<bool>{false, false, true, false, true, true, false, false},
        vector<bool>{false, false, false, true, false, false, true, false},
        vector<bool>{false, false, true, false, false, false, false, true},
        vector<bool>{true, false, false, false, false, true, false, false},
        vector<bool>{false, false, false, false, false, false, true, false},
        vector<bool>{false, false, false, false, false, true, false, true},
        vector<bool>{false, false, false, false, false, false, false, true},
    };

    vector<set<int>> sccs = scc(adj_matrix);
    CHECK(find(sccs.begin(), sccs.end(), set<int>{0, 1, 4}) != sccs.end());
    //CHECK(sccs.find(set<int>{2, 3}) != sccs.end());
    //CHECK(sccs.find(set<int>{5, 6}) != sccs.end());
    //CHECK(sccs.find(set<int>{7}) != sccs.end());
}

TEST_CASE("DFS") {

    vector<vector<bool>> adj_matrix = {
        vector<bool>{0, 1, 0, 0, 0, 0, 0, 0},
        vector<bool>{0, 0, 1, 0, 1, 1, 0, 0},
        vector<bool>{0, 0, 0, 1, 0, 0, 1, 0},
        vector<bool>{0, 0, 1, 0, 0, 0, 0, 1},
        vector<bool>{1, 0, 0, 0, 0, 1, 0, 0},
        vector<bool>{0, 0, 0, 0, 0, 0, 1, 0},
        vector<bool>{0, 0, 0, 0, 0, 1, 0, 1},
        vector<bool>{0, 0, 0, 0, 0, 0, 0, 1},
    };

    map<int, tuple<int, int, int>> visits = dfs(adj_matrix);
    // Check all discovered
    for (int v = 0; v<adj_matrix.size(); v++) {
        CHECK(visits.find(v) != visits.end());
    }
}


TEST_CASE("Extended Euclidean Algorithm") {

    // (a,b,gcd(a,b))
    list<tuple<int, int, int>> testcases = {
        tuple<int, int, int>{1, 1, 1},
        tuple<int, int, int>{2, 0, 2},
        tuple<int, int, int>{0, 2, 2},
        tuple<int, int, int>{12, 36, 12},
        tuple<int, int, int>{125546, 36546, 2},
    };

    for(tuple<int, int, int> tc: testcases) {
        int a = get<0>(tc);
        int b = get<1>(tc);
        int gcd =  get<2>(tc);
        tuple<int, int, int> got = egcd(a, b);
        CHECK(get<0>(got) == gcd);
        CHECK(get<1>(got)*a +  get<2>(got)*b == gcd);
    }

}