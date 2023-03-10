/*****************************************************************************************[Main.cc]
 Glucose -- Copyright (c) 2009, Gilles Audemard, Laurent Simon
				CRIL - Univ. Artois, France
				LRI  - Univ. Paris Sud, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose are exactly the same as Minisat on which it is based on. (see below).

---------------

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <errno.h>

#include <signal.h>
#include <zlib.h>

#include "utils/System.h"
#include "utils/ParseUtils.h"
#include "utils/Options.h"
#include "core/Dimacs.h"
#include "core/Solver.h"

#include <set>
#include <map>
#include <string>
#include <iostream>
#include <vector>

using namespace Glucose;
using namespace std;

#define LIN_EQUAL               1
#define LIN_LEQ                 2

/*
 * Some helper stuff
 * */

bool terminal(char c){
    return c >= 'a' && c <= 'z';
}

bool variable(char c){
    return c >= 'A' && c <= 'Z';
}
static BoolOption opt_newEncoding("MAIN", "newEncoding", "Use the encoding which does not depend in matching variables", true);

//=================================================================================================
map<pair<pair<int, int>, int>, Var> variableVars;
map<char, int> terminalIndices, variableIndices;
map<int, char> index2Terminal, index2Varible, var2Terminal;
vector<string> input_equations_lhs, input_equations_rhs;
vector<map<pair<int, int>, Var> > equations_lhs, equations_rhs;
map<pair<int, int>, Var> constantsVars;
map<int, int> maxPadding;
int globalMaxPadding;

vector<vector<Var> > stateTables;
vector<int> stateTableColumns, stateTableRows;

int getIndex(int numCols, int row, int col){
    return row*numCols + col;
}

// oneHotEncoding[i,j] == |X_i|=j
map<pair<int, int>, Lit> oneHotEncoding;

int sigmaSize;
int gammaSize; // Variable Alphabet size

Var trueConst, falseConst;

void readSymbols(string & s){
    for(int j = 0 ; j < s.size();j++){
        if(terminal(s[j])){
            if(terminalIndices.count(s[j]) == 0){
                terminalIndices[s[j]]=sigmaSize++;
                index2Terminal[sigmaSize-1] = s[j];
            }
        }
        else if(variableIndices.count(s[j]) == 0){
           int tmp = variableIndices.size();
           variableIndices[s[j]] = tmp;
           index2Varible[tmp] = s[j];
           maxPadding[tmp] = globalMaxPadding;
        }
    }
}

// lhs <-> /\ rhs
void reify_and(Solver & s, Lit lhs, vec<Lit> & rhs){
    assert(rhs.size() > 0 && "reifying empty list? ");
    // lhs -> rhs[i]
    for(int i = 0 ; i < rhs.size();i++){
        vec<Lit> ps;
        ps.push(rhs[i]);
        ps.push(~lhs);
        s.addClause(ps);
    }
    // /\rhs -> lhs
    vec<Lit> ps;
    for(int i = 0 ; i < rhs.size();i++)
        ps.push(~rhs[i]);
    ps.push(lhs);
    s.addClause(ps);
}

// lhs <-> \/ rhs
void reify_or(Solver & s, Lit lhs, vec<Lit> & rhs){
    assert(rhs.size() > 0 && "reifying empty list? ");
    // rhs[i] -> lhs
    for(int i = 0 ; i < rhs.size();i++){
        vec<Lit> ps;
        ps.push(~rhs[i]);
        ps.push(lhs);
        s.addClause(ps);
    }
    // lhs -> \/ rhs
    vec<Lit> ps;
    for(int i = 0 ; i < rhs.size();i++)
        ps.push(rhs[i]);
    ps.push(~lhs);
    s.addClause(ps);
}


void addOneHotEncoding(Solver & s){
    int numVars = variableIndices.size();

    assert(numVars > 0);
    for(int i = 0 ; i < numVars ; i++){
        // oneHot[i,0] <-> x_i[0]=epsilon
        oneHotEncoding[make_pair(i, 0)] = mkLit(variableVars[make_pair(make_pair(i, 0), sigmaSize)]);
        for(int j = 1 ; j < maxPadding[i] ; j++){
            Var v = s.newVar();
            vec<Lit> ps;
            assert(variableVars.count(make_pair(make_pair(i, j), sigmaSize)));
            assert(variableVars.count(make_pair(make_pair(i, j-1), sigmaSize)));
            ps.push( mkLit(variableVars[make_pair(make_pair(i, j), sigmaSize)]));              //x[j] = epsilon
            ps.push(~mkLit(variableVars[make_pair(make_pair(i, j-1), sigmaSize)]));            // x[j-1] != epsilon
            reify_and(s, mkLit(v), ps);
            oneHotEncoding[make_pair(i, j)] = mkLit(v);
        }

        // Last position: oneHot[i, max] <-> x[max] != epsilon
        if(maxPadding[i] > 0){
            assert(variableVars.count(make_pair(make_pair(i, maxPadding[i]-1), sigmaSize)));

            oneHotEncoding[make_pair(i, maxPadding[i])] = ~mkLit(variableVars[make_pair(make_pair(i, maxPadding[i]-1), sigmaSize)]);
        }

    }
    // Add a clause that at least one of the one-hot-literals must be true:
    for(int i = 0 ; i < numVars ; i++){
        vec<Lit> ps;
        for(int j = 0 ; j <= maxPadding[i] ; j++)
            ps.push(oneHotEncoding[make_pair(i, j)]);
        s.addClause(ps);
    }
}

void getCoefficients(string & lhs, string & rhs, map<int, int> & coefficients, int & c){
    assert(c == 0);
    for(int j = 0 ; j < lhs.size();j++){
        if(terminal(lhs[j])){
            c++;
        }
        else {
            assert(variableIndices.count(lhs[j]) != 0);
            coefficients[variableIndices[lhs[j]]]--;
        }
    }
    for(int j = 0 ; j < rhs.size();j++){
        if(terminal(rhs[j])){
            c--;
        }
        else {
            assert(variableIndices.count(rhs[j]) != 0);
            coefficients[variableIndices[rhs[j]]]++;
        }
    }
}

// TODO: Lin-Constraint (via MDDs)

bool addSizeEqualityConstraint(Solver & s, string & str_lhs, string & str_rhs){

    map<int, int>  coefficients;
    int rhs=0;
    getCoefficients(str_lhs, str_rhs, coefficients, rhs);

    set<pair<int, int> > states;
    int numVars = variableIndices.size();
    states.insert(make_pair(-1, 0));    // state for the empty prefix
    vector<int >  currentRow;

    map<pair<int, int>, set<pair<int, int> > > predecessors, successors;
    currentRow.push_back(0);

    set<int> nextRow;

    for(int i = 0 ; i < numVars;i++){
        for(int j = 0 ; j < currentRow.size();j++){
            for(int k = 0 ; k <= maxPadding[i] ; k++){
                int nextValue = currentRow[j] + k * coefficients[i];
                nextRow.insert(nextValue);
                states.insert(make_pair(i, nextValue));
            }
        }
        currentRow.clear();
        currentRow.insert(currentRow.end(), nextRow.begin(), nextRow.end());
        nextRow.clear();
    }
    printf("c created %d states for MDD! \n", states.size());
    /*for(set<pair<int, int> >::iterator it = states.begin() ; it != states.end();it++){
        cout << "initial state: " << it->first << " " << it->second << endl;
    }*/
    if(states.count(make_pair(numVars-1, rhs)) == 0){
        cout << "c linear equality is unsatisfiable??? " << endl;
        return false;
    }

    set<pair<int, int> > markedStates;
    vector<pair<int, int> > queue;
    int nextIndex = 0;
    // Mark final state
    markedStates.insert(make_pair(numVars-1, rhs));
    queue.push_back(make_pair(numVars-1, rhs));
    while(nextIndex < queue.size()){
        pair<int, int> currentState = queue[nextIndex];
        nextIndex++;
        int this_var = currentState.first;
        if(this_var < 0){
            // Okay, I reached the root
            markedStates.insert(currentState);
        }
        else{
            assert(this_var >= 0 && this_var < numVars);
            for(int j = 0 ; j <= maxPadding[this_var] ; j++){
                pair<int, int> predecessor = make_pair(this_var-1, currentState.second - j * coefficients[this_var]);
                if(states.count(predecessor)){
                    if(markedStates.count(predecessor) == 0)
                        queue.push_back(predecessor);

                    markedStates.insert(predecessor);
                    predecessors[currentState].insert(predecessor);
                    successors[predecessor].insert(currentState);
                }
            }
        }
    }

    printf("c have %d marked states! \n", markedStates.size());
    map<pair<int, int>, Var> partialSumVariables;
    for(set<pair<int, int> >::iterator it = markedStates.begin() ; it != markedStates.end();it++){
        partialSumVariables[*it] = s.newVar();
    }
    assert(partialSumVariables.count(make_pair(-1, 0)));
    s.addClause(mkLit(partialSumVariables[make_pair(-1, 0)]));

    assert(partialSumVariables.count(make_pair(numVars-1, rhs)));
    s.addClause(mkLit(partialSumVariables[make_pair(numVars-1, rhs)]));

    // Add clauses: A[i-1,j] /\ x_i = c -> A[i, j+a_i * c]
    for(set<pair<int, int> >::iterator it = markedStates.begin() ; it != markedStates.end();it++){
        //cout << "Have marked state " << it->first << " " << it->second << " and " << s.nFreeVars() << " free variables" << endl;
        int this_var = it->first + 1;
        if(this_var >= numVars){
            assert(this_var == numVars);
            assert(it->second == rhs);
            cout << "adding unit clause! " << endl;
            s.addClause(mkLit(partialSumVariables[*it]));
        }
        else{
            assert(maxPadding.count(this_var));
            int successorsFound = 0;
            int lastVarAssignmentThatFit = -1;
            for(int i = 0 ; i <= maxPadding[this_var] ; i++){
                int new_sum = it->second + i * coefficients[this_var];
                assert(states.count(make_pair(this_var, new_sum)));

                vec<Lit> ps;
                assert(partialSumVariables.count(*it));
                ps.push(~mkLit(partialSumVariables[*it])); // A[this_var-1,j]
                if(oneHotEncoding.count(make_pair(this_var, i)) == 0){
                    cout << "Cannot find oneHot for variable " << this_var << " and value " << i << endl;
                }
                assert(oneHotEncoding.count(make_pair(this_var, i)));
                ps.push(~oneHotEncoding[make_pair(this_var, i)]);           // this_var=i


                if(markedStates.count(make_pair(this_var, new_sum))){
                    successorsFound++;
                    assert(markedStates.count(make_pair(this_var, new_sum)));
                    assert(partialSumVariables.count(make_pair(this_var, new_sum)));
                    ps.push(mkLit(partialSumVariables[make_pair(this_var, new_sum)]));
                    lastVarAssignmentThatFit = i;
                }
                if(!s.addClause(ps)){
                    cout << "got false while adding a clause! " << endl;
                }


            }
            assert(successorsFound > 0);
            if(successorsFound == 1){
                assert(lastVarAssignmentThatFit >= 0);
                vec<Lit> ps;
                assert(partialSumVariables.count(*it));
                ps.push(~mkLit(partialSumVariables[*it])); // A[this_var-1,j]
                //printf("c only one successor, adding unit clause! \n");
                ps.push(oneHotEncoding[make_pair(this_var, lastVarAssignmentThatFit)]);    // Only one successor. Thus, if A[i,j] is active, this immediately implies the value of x[i]
                s.addClause(ps);
            }

        }

//        vector<pair<int, int> > & v = it->second;
//        for(int i = 0 ; i < v.size();i++){
//            //
//            vec<Lit> ps;
//            ps.push(~mkLit(partialSumVariables[it->first])); // A[i-1, j]
//            assert(coefficients.count(i));
//            int c = (v[i].second - it->first.second) / coefficients[i];
//            assert(it->first.second + c * coefficients[i] == v[i].second);
//        }
    }
    return true;
}

// TODO: Sets of equations (-> thus, functions for each equation)

// localOptimisation: add clauses s(i,j) -> (s(i+1, j) \/ s(i+1, j+1) \/ s(i,j+1))
void encodeEquation(Solver & S, string & input_w1, string & input_w2, bool localOptimisation, bool fillUntilSquare){
    map<pair<int, int>, Var> w1, w2;
    int szLHS = 0;
    int szRHS = 0;
    int column = 0;
    for(int i = 0 ; i < input_w1.size() ; i++){
        if(terminal(input_w1[i])){
            for(int j = 0 ; j <= sigmaSize ; j++){
                w1[make_pair(column, j)] = constantsVars[make_pair(terminalIndices[input_w1[i]], j)];
            }
            column++;
        }
        else{
            assert(variable(input_w1[i]));
            //cout << "c Looking for variable " << input_w1[i] << endl;
            assert(maxPadding.count(variableIndices[input_w1[i]]));
            for(int j = 0 ; j < maxPadding[variableIndices[input_w1[i]]] ; j++){
                for(int k = 0 ; k <= sigmaSize ; k++){
                    assert(variableVars.count(make_pair(make_pair(variableIndices[input_w1[i]], j), k)));
                    w1[make_pair(column, k)] = variableVars[make_pair(make_pair(variableIndices[input_w1[i]], j), k)];
                }
                column++;
            }
        }
    }
    szLHS = column;
    cout << "c Done with first part, have " << column << " many columns" << endl;
    column = 0;
    for(int i = 0 ; i < input_w2.size() ; i++){
        if(terminal(input_w2[i])){
            for(int j = 0 ; j <= sigmaSize ; j++){
                w2[make_pair(column, j)] = constantsVars[make_pair(terminalIndices[input_w2[i]], j)];
            }
            column++;
        }
        else{
            assert(variable(input_w2[i]));
            assert(maxPadding.count(variableIndices[input_w2[i]]));

            for(int j = 0 ; j < maxPadding[variableIndices[input_w2[i]]] ; j++){
                for(int k = 0 ; k <= sigmaSize ; k++){
                    assert(variableVars.count(make_pair(make_pair(variableIndices[input_w2[i]], j), k)));
                    w2[make_pair(column, k)] = variableVars[make_pair(make_pair(variableIndices[input_w2[i]], j), k)];
                }
                column++;
            }
        }
    }
    szRHS = column;
    bool ignorePadding = false;
    //bool fillUntilSquare = false;
    if(ignorePadding){
        if(szLHS < szRHS)
            szRHS = szLHS;
        if(szRHS < szLHS)
            szLHS = szRHS;
    }
    if(fillUntilSquare){
        if(szLHS < szRHS){
            cout << "c Padding left-hand side: " << endl;
            for( ; szLHS < szRHS ; szLHS++){
                for(int j = 0 ; j <= sigmaSize ; j++){
                    assert(w1.count(make_pair(szLHS, j)) == false);
                    w1[make_pair(szLHS, j)] = constantsVars[make_pair(sigmaSize, j)];
                }
            }
        }
        if(szLHS > szRHS){
            cout << "c Padding right-hand side: " << endl;
            for( ; szRHS < szLHS ; szRHS++){
                for(int j = 0 ; j <= sigmaSize ; j++){
                    w2[make_pair(szRHS, j)] = constantsVars[make_pair(sigmaSize, j)];
                }
            }
        }
        assert(szRHS == szLHS);
    }
    printf("c creating table of size %d x %d\n", szLHS, szRHS);
    //map<pair<int, int>, Var> stateVars;
    vector<Var> stateVars((szLHS+1) * (szRHS+1), var_Undef);
    int lastIndex = -1;
    for(int i = 0 ; i <= szLHS ; i++){
        for(int j = 0 ; j <= szRHS ; j++){
            assert(getIndex(szRHS+1, i, j) == lastIndex+1);
            lastIndex = getIndex(szRHS+1, i, j);
            assert(stateVars[getIndex(szRHS+1, i, j)] == var_Undef);
            stateVars[getIndex(szRHS+1,i, j)] = S.newVar();
        }
    }

    // Empty prefixes match
    S.addClause(mkLit(stateVars[getIndex(szRHS+1,0,0)]));
    // Final state is active
    S.addClause(mkLit(stateVars[getIndex(szRHS+1,szLHS,szRHS)]));

    if(opt_newEncoding){
        for(int i = 0 ; i < szLHS ; i++){
            for(int j = 0 ; j < szRHS ; j++){
                //assert(S.okay());
                // s(i,j) active -> exactly one of the successors is active
                assert(stateVars[getIndex(szRHS+1,i,j)] != var_Undef);
                assert(stateVars[getIndex(szRHS+1,i+1,j)] != var_Undef);
                assert(stateVars[getIndex(szRHS+1,i,j+1)] != var_Undef);
                assert(stateVars[getIndex(szRHS+1,i+1,j+1)] != var_Undef);


                vec<Lit> ps;
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                ps.push(mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                ps.push(mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                int nBefore = S.nClauses();
                S.addClause(ps);
                ps.clear();
                /*if(S.nClauses() == nBefore){
                    printf("c clause for i=%d and j=%d is ignored! \n", i, j);
                }*/

                // (i,j) is active and (i+1, j) --> none of the others
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                S.addClause(ps);
                ps.clear();

                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                S.addClause(ps);
                ps.clear();

                // (i,j) is active and (i+1, j+1) --> none of the others
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                S.addClause(ps);
                ps.clear();

                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                S.addClause(ps);
                ps.clear();
                // (i,j) is active and (i, j+1) --> none of the others
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                S.addClause(ps);
                ps.clear();

                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                S.addClause(ps);
                ps.clear();

                /////////////////////////////////////////////////////////////////////
                // s(i,j) /\ w1[i] = epsilon /\ w2[j] != epsilon -> s(i+1, j)
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(~mkLit(w1[make_pair(i,sigmaSize)]));
                ps.push(mkLit(w2[make_pair(j,sigmaSize)]));
                ps.push(mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                S.addClause(ps);
                ps.clear();

                // s(i,j) /\ w1[i] != epsilon -> NOT s(i+1, j)
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(mkLit(w1[make_pair(i,sigmaSize)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                S.addClause(ps);
                ps.clear();

                /////////////////////////////////////////////////////////////////////
                // s(i,j) /\ w1[i] != epsilon /\ w2[j] = epsilon -> s(i, j+1)
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(mkLit(w1[make_pair(i,sigmaSize)]));
                ps.push(~mkLit(w2[make_pair(j,sigmaSize)]));
                ps.push(mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                S.addClause(ps);
                ps.clear();

                // s(i,j) /\ w2[j] != epsilon -> NOT s(i, j+1)
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(mkLit(w2[make_pair(j,sigmaSize)]));
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                S.addClause(ps);
                ps.clear();

                /////////////////////////////////////////////////////////////////////
                // s(i,j) /\ w1[i] = epsilon /\ w2[j] = epsilon -> s(i+1, j+1)
                ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                ps.push(~mkLit(w1[make_pair(i,sigmaSize)]));
                ps.push(~mkLit(w2[make_pair(j,sigmaSize)]));
                ps.push(mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                S.addClause(ps);
                ps.clear();


                /////////////////////////////////////////////////////////////////////
                // s(i,j) /\ s(i+1, j+1) => w1[i] = w2[j]
                for(int k = 0 ; k <= sigmaSize ; k++){
                    ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                    ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                    ps.push(~mkLit(w1[make_pair(i,k)]));
                    ps.push(mkLit(w2[make_pair(j,k)]));
                    S.addClause(ps);
                    ps.clear();

                    ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                    ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                    ps.push(mkLit(w1[make_pair(i,k)]));
                    ps.push(~mkLit(w2[make_pair(j,k)]));
                    S.addClause(ps);
                    ps.clear();

                }
                assert(ps.size() == 0);
                if(i > 0 && j > 0){
                    ps.push(~mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                    S.addClause(ps);
                    ps.clear();
                }
            }
        }
    }
    else{
        //int equationSizes = szRHS;
        //cout << "now have equationSize " << equationSizes << endl;
        // TODO:
        // equality-predicates
        map<pair<int, int>, Var> wordsMatch;
        for(int i = 0 ; i <= szLHS ; i++){
            for(int j = 0 ; j <= szRHS ; j++){
                if(i == szLHS || j == szRHS){
                    Var v = S.newVar();
                    S.addClause(~mkLit(v));
                    wordsMatch[make_pair(i,j)] = v;
                }
                else{
                    vec<Lit> atoms;
                    for(int k = 0 ; k <= sigmaSize ; k++){
                        Var v = S.newVar();
                        atoms.push(mkLit(v));
                        // v <-> w1[i]=k /\ w2[j] = k
                        vec<Lit> ps;
                        ps.push(mkLit(w1[make_pair(i, k)]));
                        ps.push(mkLit(w2[make_pair(j, k)]));
                        reify_and(S, mkLit(v), ps);

                    }
                    Var v = S.newVar();
                    wordsMatch[make_pair(i,j)] = v;
                    // wordsMatch[i,j] = \/ atoms
                    reify_or(S, mkLit(v), atoms);
                }
            }
        }
        // automaton:
        // s[i,j] is true <-> left hand side and ride hand side matched up to indices (i-1, j-1)
        // thus, s[0,0] is always true


        // Empty prefixes match
        S.addClause(mkLit(stateVars[getIndex(szRHS+1,0,0)]));
        // Final state is active
        S.addClause(mkLit(stateVars[getIndex(szRHS+1,szLHS,szRHS)]));


        cout << "Have automaton size " << szLHS << " times " << szRHS << " and created " << stateVars.size() << " many stateVars" << endl;
        for(int i = 0 ; i <= szLHS ; i++){
            for(int j = 0 ; j <= szRHS; j++){
                vec<Lit> or_rhs;
                // Case 1: state (i-1, j-1) was active and match at position (i-1,j-1)
                if(i > 0 && j > 0){
                    Var v = S.newVar();
                    vec<Lit> ps;
                    assert(wordsMatch.count(make_pair(i-1, j-1)));
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i-1, j-1)]));
                    ps.push(mkLit(wordsMatch[make_pair(i-1,j-1)]));
                    reify_and(S,mkLit(v), ps);
                    or_rhs.push(mkLit(v));
                }

                ///////////////////////
                /// \brief v2
                ///

                if(i > 0){
                    // v2 <-> S(i-1, j) /\ !match /\ w1[i-1] = epsilon
                    Var v = S.newVar();
                    vec<Lit> ps;
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i-1, j)]));
                    assert(wordsMatch.count(make_pair(i-1, j)));
                    ps.push(~mkLit(wordsMatch[make_pair(i-1,j)]));
                    ps.push(mkLit(w1[make_pair(i-1,sigmaSize)]));

                    reify_and(S,mkLit(v), ps);
                    or_rhs.push(mkLit(v));
                }

                /////////////////////
                /// \brief v3
                ///

                if(j > 0){
                    // v3 <-> S(i, j-1) /\ !match /\ w2[j-1] = epsilon
                    Var v = S.newVar();
                    vec<Lit> ps;
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i, j-1)]));
                    assert(wordsMatch.count(make_pair(i, j-1)));
                    ps.push(~mkLit(wordsMatch[make_pair(i,j-1)]));
                    ps.push(mkLit(w2[make_pair(j-1,sigmaSize)]));

                    reify_and(S,mkLit(v), ps);
                    or_rhs.push(mkLit(v));
                }

                // S(i,j) <-> v1 \/ v2 \/ v3 (if all of them exist)

                if(or_rhs.size() == 0){
                    assert(i == 0 && j == 0);
                }
                else if(or_rhs.size() < 3){
                    assert(or_rhs.size() == 1 && (i == 0 || j == 0));
                }

                if(or_rhs.size() > 0){
                    reify_or(S, mkLit(stateVars[getIndex(szRHS+1,i,j)]), or_rhs);
                }
            }
        }
        if(localOptimisation){
            for(int i = 0 ; i < szLHS ; i++){
                for(int j = 0 ; j < szRHS ; j++){
                    assert(stateVars[getIndex(szRHS+1,i,j)] != var_Undef);
                    assert(stateVars[getIndex(szRHS+1,i+1,j)] != var_Undef);
                    assert(stateVars[getIndex(szRHS+1,i,j+1)] != var_Undef);
                    assert(stateVars[getIndex(szRHS+1,i+1,j+1)] != var_Undef);
                    vec<Lit> ps;
                    ps.push(~mkLit(stateVars[getIndex(szRHS+1,i,j)]));
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i+1,j)]));
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i,j+1)]));
                    ps.push(mkLit(stateVars[getIndex(szRHS+1,i+1,j+1)]));
                    S.addClause(ps);
                }
            }
        }
        for(map<pair<int, int>, Var>::iterator it = wordsMatch.begin() ; it != wordsMatch.end();it++){
            S.setDecisionVar(it->second, false);
        }
    }


    for(vector<Var>::iterator it = stateVars.begin() ; it != stateVars.end();it++){
        if(*it != var_Undef)
            S.setDecisionVar(*it, false);
    }

    equations_lhs.push_back(w1);
    equations_rhs.push_back(w2);
    stateTables.push_back(stateVars);
    stateTableColumns.push_back(szRHS+1);
    stateTableRows.push_back(szLHS+1);
}


// Inequality between variables
void encodeNotEqual(Solver & s, int firstIndex, int secondIndex, int sigmaSize){
    cout << "c adding inequality between " << firstIndex << " and " << secondIndex << endl;

    vec<Lit> diffVars;          // \/ (not matchHere(i) )
    assert(maxPadding.count(firstIndex));
    assert(maxPadding.count(secondIndex));
    int maxVarSize = std::min(maxPadding[firstIndex], maxPadding[secondIndex]);
    for(int i = 0 ; i < maxVarSize ; i++){
        Var matchHere = s.newVar();
        vec<Lit> match_rhs;
        for(int k = 0 ; k <= sigmaSize ; k++){
            // v <-> x[i]=k  /\ y[i]=k
            Var v = s.newVar();
            vec<Lit> ps;
            assert(variableVars.count(make_pair(make_pair(firstIndex, i), k)));
            assert(variableVars.count(make_pair(make_pair(secondIndex, i), k)));
            ps.push(mkLit(variableVars[make_pair(make_pair(firstIndex, i), k)]));
            ps.push(mkLit(variableVars[make_pair(make_pair(secondIndex, i), k)]));
            reify_and(s,mkLit(v), ps);
            match_rhs.push(mkLit(v));
        }
        reify_or(s,mkLit(matchHere), match_rhs);
        diffVars.push(~mkLit(matchHere));
    }
    // TODO: Make sure this also works if sizes are not equal:
    if(maxPadding[firstIndex] > maxPadding[secondIndex]){
        assert(variableVars.count(make_pair(make_pair(firstIndex, maxPadding[secondIndex]), sigmaSize)));
        diffVars.push(~mkLit(variableVars[make_pair(make_pair(firstIndex, maxPadding[secondIndex]), sigmaSize)]));
    }
    else if(maxPadding[firstIndex] < maxPadding[secondIndex]){
        assert(variableVars.count(make_pair(make_pair(secondIndex, maxPadding[firstIndex]), sigmaSize)));
        diffVars.push(~mkLit(variableVars[make_pair(make_pair(secondIndex, maxPadding[firstIndex]), sigmaSize)]));
    }
    s.addClause(diffVars);
}


 void sharpenBounds(Solver & s, string & lhs, string & rhs){
     map<int, int> coefficients;
     int c = 0;
    getCoefficients(lhs, rhs, coefficients, c);
    cout << "c Got equation ";
    for(map<int, int>::iterator it = coefficients.begin() ; it != coefficients.end();it++){
        cout << it->second << " * " << index2Varible[it->first] << " ";
    }
    cout << "= " << c << endl;
    for(map<int, int>::iterator it = coefficients.begin() ; it != coefficients.end();it++){
        if(it->second != 0){ // only consider unbalanced variables!
            int rhs = c;
            for(map<int, int>::iterator others = coefficients.begin() ; others != coefficients.end();others++){
                if((it->second < 0) ^ (others->second < 0)){ // get upper bound, thus try to make rhs/a_i as large as possible

                    rhs -= others->second * maxPadding[others->first];
                }
            }

            cout << "c Can infer bound " << index2Varible[it->first] << " <= " << rhs << "/" << it->second << " = " << (rhs / it->second) << endl;
            rhs /= it->second;
            if(rhs < maxPadding[it->first])
                maxPadding[it->first] = rhs;
        }
    }

 }


// sum a_i x_i - sum b_j x_j <=  + c, where a_i, b_j >= 0

// Expect x_i as one-hot encoded, with pairs (j, v_j) <-> x_i = j

void encodeLinConstraint(Solver & s, vec<int> & ai, vector<vector<pair<int, Lit> > > & xi, int c, int type){
    map<pair<int, int>, Var > partialSums;
    Var emptySumZero = trueConst;
    partialSums[make_pair(0, 0)] = trueConst;
    for(int i = 0 ; i < xi.size();i++){

    }

}

// Variables from regular language

void printStats(Solver& solver)
{
    double cpu_time = cpuTime();
    double mem_used = 0;//memUsedPeak();
    printf("c restarts              : %"PRIu64" (%"PRIu64" conflicts in avg)\n", solver.starts,(solver.starts>0 ?solver.conflicts/solver.starts : 0));
    printf("c blocked restarts      : %"PRIu64" (multiple: %"PRIu64") \n", solver.nbstopsrestarts,solver.nbstopsrestartssame);
    printf("c last block at restart : %"PRIu64"\n",solver.lastblockatrestart);
    printf("c nb ReduceDB           : %lld\n", solver.nbReduceDB);
    printf("c nb removed Clauses    : %lld\n",solver.nbRemovedClauses);
    printf("c nb learnts DL2        : %lld\n", solver.nbDL2);
    printf("c nb learnts size 2     : %lld\n", solver.nbBin);
    printf("c nb learnts size 1     : %lld\n", solver.nbUn);

    printf("c conflicts             : %-12"PRIu64"   (%.0f /sec)\n", solver.conflicts   , solver.conflicts   /cpu_time);
    printf("c decisions             : %-12"PRIu64"   (%4.2f %% random) (%.0f /sec)\n", solver.decisions, (float)solver.rnd_decisions*100 / (float)solver.decisions, solver.decisions   /cpu_time);
    printf("c propagations          : %-12"PRIu64"   (%.0f /sec)\n", solver.propagations, solver.propagations/cpu_time);
    printf("c conflict literals     : %-12"PRIu64"   (%4.2f %% deleted)\n", solver.tot_literals, (solver.max_literals - solver.tot_literals)*100 / (double)solver.max_literals);
    printf("c nb reduced Clauses    : %lld\n",solver.nbReducedClauses);

    if (mem_used != 0) printf("Memory used           : %.2f MB\n", mem_used);
    printf("c CPU time              : %g s\n", cpu_time);
}



static Solver* solver;
// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
static void SIGINT_interrupt(int signum) { solver->interrupt(); }

// Note that '_exit()' rather than 'exit()' has to be used. The reason is that 'exit()' calls
// destructors and may cause deadlocks if a malloc/free function happens to be running (these
// functions are guarded by locks for multithreaded use).
static void SIGINT_exit(int signum) {
    printf("\n"); printf("*** INTERRUPTED ***\n");
    if (solver->verbosity > 0){
        printStats(*solver);
        printf("\n"); printf("*** INTERRUPTED ***\n"); }
    _exit(1); }


//=================================================================================================
// Main:


int main(int argc, char** argv)
{
    setbuf(stdout, NULL);
  printf("c\nc This is glucose 3.0 --  based on MiniSAT (Many thanks to MiniSAT team)\nc\n");
    try {
        setUsageHelp("c USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or gzipped DIMACS.\n");
        // printf("This is MiniSat 2.0 beta\n");

#if defined(__linux__)
        fpu_control_t oldcw, newcw;
        _FPU_GETCW(oldcw); newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE; _FPU_SETCW(newcw);
        printf("c WARNING: for repeatability, setting FPU to use double precision\n");
#endif
        // Extra options:
        //
        IntOption    verb   ("MAIN", "verb",   "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
        BoolOption   mod   ("MAIN", "model",   "show model.", false);
        IntOption    vv  ("MAIN", "vv",   "Verbosity every vv conflicts", 10000, IntRange(1,INT32_MAX));
        IntOption    cpu_lim("MAIN", "cpu-lim","Limit on CPU time allowed in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX));
        IntOption    mem_lim("MAIN", "mem-lim","Limit on memory usage in megabytes.\n", INT32_MAX, IntRange(0, INT32_MAX));
        BoolOption   squareAuto   ("MAIN", "useSquareAutomaton",   "Make the automaton the shape of a square.", false);
        BoolOption   optPrintTable   ("MAIN", "printTables",   "Print tables.", true);

        parseOptions(argc, argv, true);

        Solver S;
        double initial_time = cpuTime();

        S.verbosity = verb;
        S.verbEveryConflicts = vv;
	S.showModel = mod;
        solver = &S;
        // Use signal handlers that forcibly quit until the solver will be able to respond to
        // interrupts:
	//        signal(SIGINT, SIGINT_exit);
        //signal(SIGXCPU,SIGINT_exit);

        // Set limit on CPU-time:
        if (cpu_lim != INT32_MAX){
            rlimit rl;
            getrlimit(RLIMIT_CPU, &rl);
            if (rl.rlim_max == RLIM_INFINITY || (rlim_t)cpu_lim < rl.rlim_max){
                rl.rlim_cur = cpu_lim;
                if (setrlimit(RLIMIT_CPU, &rl) == -1)
                    printf("c WARNING! Could not set resource limit: CPU-time.\n");
            } }

        // Set limit on virtual memory:
        if (mem_lim != INT32_MAX){
            rlim_t new_mem_lim = (rlim_t)mem_lim * 1024*1024;
            rlimit rl;
            getrlimit(RLIMIT_AS, &rl);
            if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max){
                rl.rlim_cur = new_mem_lim;
                if (setrlimit(RLIMIT_AS, &rl) == -1)
                    printf("c WARNING! Could not set resource limit: Virtual memory.\n");
            } }


        trueConst = S.newVar();
        S.addClause(mkLit(trueConst));
        falseConst = S.newVar();
        S.addClause(~mkLit(falseConst));

        // Header format: maxPadding e d lin reg
        // maxPadding: maximum variable length
        // e: number of equalities
        // d: number of disequalities
        // lin: number of linear equalities
        // reg: number of regular constraints
        int e, d, lin, reg;  // upper bound on length of variables

        if(! (cin >> globalMaxPadding))
            assert(false && "invalid input format! ");
        if(! (cin >> e))
            assert(false && "invalid input format! ");
        if(! (cin >> d))
            assert(false && "invalid input format! ");
        if(! (cin >> lin))
            assert(false && "invalid input format! ");
        if(! (cin >> reg))
            assert(false && "invalid input format! ");


        for(int i = 0 ; i < e ; i++){
            string s1;
            cin >> s1;
            input_equations_lhs.push_back(s1);
            string s2;
            cin >> s2;
            input_equations_rhs.push_back(s2);
            cout << "Read " << s1 << " = " << s2 << endl;
        }


        assert(lin == 0 && "No linears yet! ");
        assert(reg == 0 && "No regulars yet! ");

        // Encode problem here
        // assume for aXbY, i.e. terminal symbols small, variables capital letters
        for(int i = 0 ; i < input_equations_lhs.size();i++){
            readSymbols(input_equations_lhs[i]);
            readSymbols(input_equations_rhs[i]);

        }
        sigmaSize = terminalIndices.size();
        // TODO: Optimise order!

        for(int i = 0 ; i < input_equations_lhs.size();i++){
            // TODO: Derive bounds on lengths here?
            sharpenBounds(S, input_equations_lhs[i], input_equations_rhs[i]);
        }



        int numVars;
        index2Terminal[sigmaSize] = '_';
        numVars = variableIndices.size();
        cout << "Okay, starting the shit. Have sigmaSize=" << sigmaSize << " and " << numVars << " variables" << endl;
        for(int i = 0 ; i < numVars ; i++){
            cout << "bound for " << index2Varible[i] << ": " << maxPadding[i] << endl;
        }
        // Encode variables for terminal symbols

        for(int i = 0 ; i <= sigmaSize ; i++){
            for(int j = 0 ; j <= sigmaSize ; j++){
                Var v = S.newVar();
                constantsVars[make_pair(i,j)] = v;
                // Make variable "true" if i=j, and false otherwise
                if(i == j)
                    S.addClause(mkLit(v));
                else
                    S.addClause(~mkLit(v));
            }
        }

        // Take a variable, and index and a sigma, and return if the variable at index "i" equals sigma
        // g:  x, i, sigma -> BV

        for(int i = 0 ; i < numVars ; i++){
            assert(maxPadding.count(i));
            for(int j = 0 ; j < maxPadding[i] ; j++){
                for(int k = 0 ; k <= sigmaSize ; k++){
                    Var v = S.newVar();
                    variableVars[make_pair(make_pair(i, j), k)] = v;
                }
            }
            // Assert that epsilons occur at the end of a substitution
            for(int j = 0 ; j + 1 < maxPadding[i] ; j++){
                S.addClause(~mkLit(variableVars[make_pair(make_pair(i, j), sigmaSize)]), mkLit(variableVars[make_pair(make_pair(i, j+1), sigmaSize)]) );
            }
        }

        // Alldifferent: Make sure that each variable is assigned to exactly one letter from Sigma (or epsilon)
        // TODO: Do this with linear number of clauses (!!!)
        for(int i = 0 ; i < numVars ; i++){
            assert(maxPadding.count(i));
            for(int j = 0 ; j < maxPadding[i] ; j++){
                vec<Lit> ps;
                for(int k = 0 ; k <= sigmaSize ; k++){
                    assert(variableVars.count(make_pair(make_pair(i, j), k)));
                    ps.push(mkLit(variableVars[make_pair(make_pair(i, j), k)]));
                    for(int l = k+1 ; l <= sigmaSize; l++){
                        assert(variableVars.count(make_pair(make_pair(i, j), l)));
                        S.addClause(~mkLit(variableVars[make_pair(make_pair(i, j), k)]),~mkLit(variableVars[make_pair(make_pair(i, j), l)]));
                    }
                }
                S.addClause(ps);
            }
        }
        addOneHotEncoding(S);

        for(int i = 0 ; i < input_equations_lhs.size();i++){
            bool succ = addSizeEqualityConstraint(S, input_equations_lhs[i], input_equations_rhs[i]);
            if(!succ){
                printf("c UNSAT\n");
                exit(20);
            }
        }

        // Encode inequalities:
        // parse other stuff
        for(int i = 0 ; i < d ; i++){
            string lhs;
            cin >> lhs;
            string rhs;
            cin >> rhs;
            assert(variableIndices.count(lhs[0]));
            assert(variableIndices.count(rhs[0]));
            encodeNotEqual(S, variableIndices[lhs[0]], variableIndices[rhs[0]], sigmaSize);
        }

        for(int i = 0 ; i < input_equations_lhs.size();i++){
            cout << "Now encoding stuff for words " << input_equations_lhs[i] << " and " << input_equations_rhs[i] << endl;
            encodeEquation(S, input_equations_lhs[i], input_equations_rhs[i], true, squareAuto);
        }




        if (S.verbosity > 0){
            printf("c |  Number of variables:  %12d                                                                   |\n", S.nVars());
            printf("c |  Number of clauses:    %12d                                                                   |\n", S.nClauses()); }

        double parsed_time = cpuTime();
        if (S.verbosity > 0){
            printf("c |  Parse time:           %12.2f s                                                                 |\n", parsed_time - initial_time);
            printf("c |                                                                                                       |\n"); }

        // Change to signal-handlers that will only notify the solver and allow it to terminate
        // voluntarily:
        //signal(SIGINT, SIGINT_interrupt);
        //signal(SIGXCPU,SIGINT_interrupt);

        if (!S.simplify()){
            if (S.certifiedOutput != NULL) fprintf(S.certifiedOutput, "0\n"), fclose(S.certifiedOutput);
            if (S.verbosity > 0){
	        printf("c =========================================================================================================\n");
                printf("Solved by unit propagation\n");
                printStats(S);
                printf("\n"); }
            printf("s UNSATISFIABLE\n");
            exit(20);
        }

        vec<Lit> dummy;
        printf("c time for setting up everything: %lf\n", cpuTime());
        printf("c okay=%d\n", S.okay());
        lbool ret = S.solveLimited(dummy);
        if (S.verbosity > 0){
            printStats(S);
            printf("\n"); }

        int stateVarsSeen = 0;
        int stateVarsOverall = 0;
        if(optPrintTable){
            for(int t = 0 ; t < stateTables.size();t++){
                vector<Var> & v = stateTables[t];
                int nCols = stateTableColumns[t];
                int nRows = stateTableRows[t];
                cout << "print a " << nRows << " x " << nCols << " matrix..." << endl;
                int index = 0;
                for(int i = 0 ; i < nRows ; i++){
                    for(int j = 0 ; j < nCols ; j++){
                        stateVarsOverall++;
                        assert(index == getIndex(nCols, i, j));
                        if(S.varSeen[v[getIndex(nCols, i, j)]]){
                            stateVarsSeen++;
                            cout << "*";
                        }
                        else {
                            cout << " ";
                        }
                        index++;
                    }
                    cout << "|" << i << endl;
                }
                assert(index == v.size());
                cout << endl;
            }
        }
        printf("c saw %d out of %d state variables! \n", stateVarsSeen, stateVarsOverall);

	//-------------- Result is put in a external file

	  printf(ret == l_True ? "s SATISFIABLE\n" : ret == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");
	  if(S.showModel && ret==l_True) {
	    printf("v ");
	    for (int i = 0; i < S.nVars(); i++)
	      if (S.model[i] != l_Undef)
		printf("%s%s%d", (i==0)?"":" ", (S.model[i]==l_True)?"":"-", i+1);
	    printf(" 0\n");
	  }

          if(ret == l_True){
              cout << "Okay, variables are substituted with: " << endl;
              for(int i = 0 ; i < numVars ; i++){
                  assert(maxPadding.count(i));
                  cout << "[SUBSITUTION] " << index2Varible[i] << " " ;
                  for(int j = 0 ; j < maxPadding[i] ; j++){
                      for(int k = 0 ; k < sigmaSize ; k++){
                          if(S.modelValue(variableVars[make_pair(make_pair(i, j), k)]) == l_True){
                              cout << index2Terminal[k];
                          }
                      }
                  }
                  cout << endl;
              }

              for(int i = 0 ; i < equations_lhs.size();i++){
                  cout << "---" << endl;
                  cout << " equation " << i << ": " << endl;
                  for(int j = 0 ; equations_lhs[i].count(make_pair(j, 0)) ; j++){
                      for(int k = 0 ; k < sigmaSize ; k++){
                          assert(equations_lhs[i].count(make_pair(j,k)));
                          if(S.modelValue(equations_lhs[i][make_pair(j,k)]) == l_True)
                              cout << index2Terminal[k];
                      }
                  }
                  cout << " = " ;
                  for(int j = 0 ; equations_rhs[i].count(make_pair(j, 0)) ; j++){
                      for(int k = 0 ; k < sigmaSize ; k++){
                          assert(equations_rhs[i].count(make_pair(j,k)));
                          if(S.modelValue(equations_rhs[i][make_pair(j,k)]) == l_True)
                              cout << index2Terminal[k];
                      }
                  }
                  cout << endl;
              }
                if(optPrintTable){
                  for(int t = 0 ; t < stateTables.size();t++){
                      vector<Var> & v = stateTables[t];
                      int index = 0;
                      for(int i = 0 ; i < stateTableRows[t] ; i++){
                          for(int j = 0 ; j < stateTableColumns[t] ; j++){

                              assert(index == getIndex(stateTableColumns[t], i, j));
                              if(S.modelValue(v[index]) == l_True){
                                  cout << "*";
                              }
                              else if(S.modelValue(v[index]) == l_False){
                                  cout << " ";
                              }
                              else
                                  cout << "?";
                              index++;
                          }
                          cout << "|" << i << endl;
                      }
                      cout << endl;
                  }
              }


          }



#ifdef NDEBUG
        exit(ret == l_True ? 10 : ret == l_False ? 20 : 0);     // (faster than "return", which will invoke the destructor for 'Solver')
#else
        return (ret == l_True ? 10 : ret == l_False ? 20 : 0);
#endif
    } catch (OutOfMemoryException&){
      printf("c ===================================================================================================\n");
        printf("INDETERMINATE\n");
        exit(0);
    }
}
