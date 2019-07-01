#ifndef _GRAPH_
#define _GRAPH_

#include <string>
#include <unordered_map>
#include <stack>

#include "words/words.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {
	  struct Edge;
	  struct Node {
		Node (const std::shared_ptr<Words::Options>& o) : opt(o) {}
		std::shared_ptr<Words::Options> opt;
		std::vector<Edge*> incoming;
		bool isRoot () {return !incoming.size();}
		bool ranSMTSolver = false;
	  };
	  
	  struct Edge {
		Edge (Node* f, Node* t,const Words::Substitution& subs) : from(f),
																  to (t),
																  subs(subs) {}
			  
		Node* from;
		Node * to;
		Words::Substitution subs;
	  };
	  
	  class Graph {
	  public:
		Node* makeNode (const std::shared_ptr<Words::Options>& opt) {
		  auto hash = opt->eqhash (0);
		  auto it = nodes.find (hash);
		  if (it != nodes.end()) {
			return it->second.get();
		  }
		  else {
			auto nnode = std::make_unique<Node> (opt);
			auto res = nnode.get ();
			nodes.insert (std::make_pair (hash,std::move(nnode)));
			return res;
		  }
		}

		Node* makeDummyNode () {
		  auto hash = 0;
		  auto it = nodes.find (hash);
		  
		  auto nnode = std::make_unique<Node> (nullptr);
		  auto res = nnode.get ();
		  nodes.insert (std::make_pair (hash,std::move(nnode)));
		  return res;
		}

		
		Node* getNode (const Words::Options& n) {
		  return nodes.find(n.eqhash(0))->second.get();
		}

		Edge* addEdge (Node* from, Node* to,const Words::Substitution& s) {
		  auto edge = std::make_unique<Edge> (from,to,s);
		  to->incoming.push_back (edge.get());
		  edges.push_back(std::move(edge));
		  return edges.back().get();
		}
		
		auto begin () const {return edges.begin();}
		auto end () const {return edges.end();}
		
	  private:
		std::unordered_map<uint32_t,std::unique_ptr<Node>> nodes;
		std::vector<std::unique_ptr<Edge> > edges;
	  };

	  inline Words::Substitution replaceInSub (const Words::Substitution& orig, const Words::Substitution& solution) {		
        Words::Substitution nnew = orig;
        std::vector<std::pair<IEntry*,IEntry*>> varEquals;

        for (auto& elem : orig) {
            if (elem.second.characters() < 1)
                continue;

            auto subsb = elem.second.ebegin();
            auto subse = elem.second.eend();

            for (auto it = subsb; it != subse; ++it) {
                if((*it)->isVariable() && (*it) != elem.first ){
                    //std::cout << "works..." << (*it)->getRepr() << " " << elem.first->getRepr() << std::endl;
                    if (solution.count(*it)){
                        //std::cout << "Okay, it's " << solution.at(*it) << std::endl;
                        nnew[elem.first].substitudeVariable(*it,solution.at(*it));
                    } else {
                        //std::cout << "Okay, not present!" << std::endl;
                        nnew[elem.first].substitudeVariable(*it,Words::Word());
                    }
                }
            }
        }

		for (const auto& elem : solution) {
          if (nnew.count(elem.first))
			nnew[elem.first].substitudeVariable (elem.first,elem.second);
          else
			nnew[elem.first] = elem.second;
        }

        // set open substitutions to the empty word
        for (auto& oelem : nnew) {
              oelem.second.substitudeVariable(oelem.first,Words::Word());
        }

		return nnew;
      }
	  
	  inline Words::Substitution findRootSolution (Node* n) {
		struct SearchNode {
		  SearchNode (Node* n, const Words::Substitution& s) : n(n), subs(s) {} 
		  Node* n;
		  Words::Substitution subs;
		};
		Words::Substitution final;
		std::stack<SearchNode> waiting;
		waiting.push(SearchNode (n,final));
		
		while (waiting.size()) {
		  auto cur = waiting.top();
		  waiting.pop();
		  if (cur.n->isRoot()) {
			return cur.subs;
		  }
		  else {
			for (auto edge : cur.n->incoming) {
			  Words::Substitution subs = replaceInSub (edge->subs,cur.subs);
			  waiting.push (SearchNode (edge->from,subs));
			}
		  }
		}
		throw "Shouldn't get here";
		
	  }

	  void outputToString (const std::string&, const Graph& g);
	  struct GuaranteeOutput {
		GuaranteeOutput (const std::string& f, const Graph& g) : file(f),g(g) {
		}
		~GuaranteeOutput () {
		  outputToString (file,g);
		}
		const std::string file;
		const Graph& g;
	  };
	}
  }
}


#endif
