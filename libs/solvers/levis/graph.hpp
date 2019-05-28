#ifndef _GRAPH_
#define _GRAPH_

#include <unordered_map>
#include <stack>

#include "words/words.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {
	  struct Edge;
	  struct Node {
		Node (const std::shared_ptr<Words::Options>& ) : opt(opt) {}
		std::shared_ptr<Words::Options> opt;
		std::vector<Edge*> incoming;
		bool isRoot () {return !incoming.size();}
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

		Node* getNode (const Words::Options& n) {
		  return nodes.find(n.eqhash(0))->second.get();
		}

		void addEdge (Node* from, Node* to,const Words::Substitution& s) {
		  auto edge = std::make_unique<Edge> (from,to,s);
		  to->incoming.push_back (edge.get());
		  edges.push_back(std::move(edge));
		}

		
		
	  private:
		std::unordered_map<uint32_t,std::unique_ptr<Node>> nodes;
		std::vector<std::unique_ptr<Edge> > edges;
	  };

	  Words::Substitution replaceInSub (const Words::Substitution& orig, const Words::Substitution& solution) {
		std::cerr << "Replace in Sub" << std::endl;
		std::cerr << "Solution" << solution << std::endl;
		std::cerr << "transition" << orig << std::endl;
		
		Words::Substitution nnew = orig;
		for (const auto& elem : solution) {
		  //std::cerr << *elem.first << std::endl;
		  if (nnew.count(elem.first)) 
			nnew[elem.first].substitudeVariable (elem.first,elem.second);
		  else
			nnew[elem.first] = elem.second;
		  //elem.second.substitudeVariable (elem.first,solution.at(elem.first));
		  //std::cerr << elem.second << std::endl;
		}
		std::cerr << "Replace " << nnew;
		return nnew;
	  }
	  
	  Words::Substitution findRootSolution (Node* n, Words::Substitution& final) {
		struct SearchNode {
		  SearchNode (Node* n, const Words::Substitution& s) : n(n), subs(s) {} 
		  Node* n;
		  Words::Substitution subs;
		};
	   
		std::stack<SearchNode> waiting;
		waiting.push(SearchNode (n,final));
		
		while (waiting.size()) {
		  std::cerr << "BackTrack" << std::endl;
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
	}
  }
}


#endif
