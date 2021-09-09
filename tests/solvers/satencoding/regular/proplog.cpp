
#include "catch2/catch.hpp"
#include "../lib/proplog.cpp"

#include <set>

using namespace PropositionalLogic;
using namespace std;

TEST_CASE("Valid formulae") {


}

TEST_CASE("Valdity") {

    CHECK(PLFormula::lit(1).valid());
    CHECK(PLFormula::lor(vector<PLFormula>{PLFormula::lit(1), PLFormula::lit(-1)}).valid());
    CHECK(PLFormula::land(vector<PLFormula>{PLFormula::lit(1), PLFormula::lit(-1)}).valid());
    CHECK(PLFormula::lnot(PLFormula::lit(1)).valid());
    CHECK(PLFormula::lnot(PLFormula::lor(vector<PLFormula>{PLFormula::lit(1), PLFormula::lit(-1)})).valid());
    

}

TEST_CASE("Nonnegative Normalform") {

    CHECK(PLFormula::lnot(PLFormula::lit(1)).is_nenf());
    CHECK(PLFormula::land(vector<PLFormula>{PLFormula::lnot(PLFormula::lit(1)), PLFormula::lit(1)}).is_nenf());
    CHECK(!PLFormula::lnot(PLFormula::lor(vector<PLFormula>{PLFormula::lit(1), PLFormula::lit(-1)})).is_nenf());

}

TEST_CASE("Depth") {

    CHECK(PLFormula::lit(1).depth() == 1);
    CHECK(PLFormula::lor(vector<PLFormula>{PLFormula::lit(1), PLFormula::lit(-1)}).depth() == 2);
    CHECK(PLFormula::lnot(PLFormula::lit(1)).depth() == 2);
    CHECK(PLFormula::lnot(
        PLFormula::lor(
            vector<PLFormula>{PLFormula::lit(1), PLFormula::lnot(PLFormula::lit(-1))}
        )
    ).depth() == 4);

}

TEST_CASE("Max var") {

    CHECK(PLFormula::lit(1).max_var() == 1);
    CHECK(PLFormula::lit(-1).max_var() == 1);
    CHECK(PLFormula::lit(-123).max_var() == 123);
    CHECK(PLFormula::lor(vector<PLFormula>{PLFormula::lit(5), PLFormula::lit(-12)}).max_var() == 12);
    CHECK(PLFormula::lor(vector<PLFormula>{PLFormula::lit(-12), PLFormula::lit(452)}).max_var() == 452);
}

TEST_CASE("Tseytin") {
    PLFormula phi = PLFormula::land(vector<PLFormula>{
        PLFormula::lor(vector<PLFormula>{
            PLFormula::lit(1),
            PLFormula::land(vector<PLFormula>{
                PLFormula::lit(2),
                PLFormula::lit(-3),
            })
        }),
        PLFormula::lit(4)
    });

    
    set<set<int>> cnf = tseytin_cnf(phi);

    CHECK(cnf.size() == 10);
    CHECK( (cnf.find(set<int>{1, 5,-6}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{5, 3, -2}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{-1, 6}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{-5, 6}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{6, -7}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{7}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{7, -6, -4}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{2, -5}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{4, -7}) != cnf.end()) );
    CHECK( (cnf.find(set<int>{-5, -3}) != cnf.end()) );
      
}