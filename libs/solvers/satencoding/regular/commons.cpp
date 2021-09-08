#include <tuple>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <numeric>
#include <functional>
#include <algorithm>

using namespace std;

namespace commons {
    
    
    int dfs_visit(int vertex, int t, int predecessor, map<int, tuple<int, int, int>> &visits, vector<vector<bool>> &adjacency_matrix) {
        t+=1;
        int t_start = t;
        // Already visited vertex, don't know actual values yet
        visits[vertex] = tuple<int, int, int>{-1, -1, -1};
        for (int v = 0; v < adjacency_matrix.size(); v++) {
            if(adjacency_matrix[vertex][v] && visits.find(v) == visits.end()) {
                // Is successor and has not been visited
                t = dfs_visit(v, t, vertex, visits, adjacency_matrix);
            }
        }
        t++;
        // Update values
        visits[vertex] = tuple<int, int, int>{t_start, t, predecessor};
        return t;
    }
    

    map<int, tuple<int, int, int>> dfs(vector<vector<bool>> adjacency_matrix, function<bool(int, int)> comparator = [](int a, int b){return a<b;}) {
        
        map<int, tuple<int, int, int>> visits;
                
        int n = adjacency_matrix.size();
        int t = 0;
        vector<int> vertices(n);
        iota(vertices.begin(), vertices.end(), 0);
        sort(vertices.begin(), vertices.end(), comparator);


        for (int vertex: vertices) {
            if (visits.find(vertex) == visits.end()) {
                dfs_visit(vertex, t, -1, visits, adjacency_matrix);
            } 
        }
        return visits;

    }

  


    

    vector<set<int>> scc(vector<vector<bool>> adjacency_matrix){
        vector<vector<bool>> transpose_adjacency_matrix;
        int n = adjacency_matrix.size();
        // init transpose with false
        for (int i=0; i<n; i++) {
            transpose_adjacency_matrix.push_back(vector<bool>(n, false));
        }
        // set transpose edges to true
        for (int i=0; i<n; i++) {
            for (int j=0; j<n; j++) {
                if (adjacency_matrix[i][j]) {
                    transpose_adjacency_matrix[j][i] = true;
                }
            }
        }



        map<int, tuple<int, int, int>> visits = dfs(adjacency_matrix);
  
        map<int, tuple<int, int, int>> visits_transpose = dfs(transpose_adjacency_matrix, [&visits](int a, int b){return get<1>(visits[a]) > get<1>(visits[b]);});

        
        int scc_counter = 0;
        map<int, int> sccs_map;
             
        for (auto it = visits_transpose.begin(); it != visits_transpose.end(); it++) {
            int pred = get<2>(it->second);
            int current = it->first;
            
            if (sccs_map.find(pred) == sccs_map.end()) {
                while (pred != -1 && sccs_map.find(pred) == sccs_map.end()) {
                    current = pred;
                    pred = get<2>(visits[pred]);
                }
            }
              
            if (pred == -1) {
                sccs_map[current] = scc_counter;
                scc_counter++;
            } else {
                sccs_map[current] = sccs_map[pred];   
            }
            
        }

        vector<set<int>> sccs(scc_counter ,set<int>{});

        int ff = 0;
        for (auto it = sccs_map.begin(); it != sccs_map.end(); it++) {
            sccs[it->second].insert(it->first);
        }
        return sccs;
    }

    /**
     * Calculate all subsets of a (multi-)set given as a vector.
     * Returns the powerset as a vector of vectors.
     */
    template <typename T>
    vector<vector<T>> powerset(vector<T> theset) {
        vector<vector<T>> powersets;
        
        for (int i=0; i<pow(2, theset.size()); i++) {
            vector<T> current;
            for (int c=0; c<theset.size(); c++) {
                int comp = 1 << c;
                if (i & comp) { // Bitwise compare at c-th position
                    current.push_back(theset[c]);
                }
            }
            powersets.push_back(current);
        }

        return powersets;
    }


    /**
     * Extended euclidean algorithm.
     * Returns a tuple (gcd(a,b), x, y) of integers such that ax + by = gcd(a,b)
     */
    tuple<int, int, int> egcd(int a, int b) {
        int aa = a;
        int bb = b;
        int x0 = 0, x1 = 1, y0 = 1, y1 = 0;
        while (a != 0) {
            // Euclid
            int q = (int)b/a;
            int tmp_a = b%a;
            

            b = a;
            a = tmp_a;

            // Update coefficients
            int y0_tmp = y0;
            y0 = y1;
            y1 = y0_tmp - q*y1;

            int x0_tmp = x0;
            x0 = x1;
            x1 = x0_tmp - q*x1;

        }
        return make_tuple(b, x0, y0);
    }



}
