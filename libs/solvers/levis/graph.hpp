#ifndef _GRAPH_
#define _GRAPH_

#include <unordered_map>

#include "words/words.hpp"

namespace Words {
  namespace Solvers {
	namespace Levis {
	  struct Edge;
	  struct Node {
		Node (const std::shared_ptr<Words::Options>& ) : opt(opt) {}
		std::shared_ptr<Words::Options> opt;
		std::vector<Edge*> incoming;
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

		auto findWayBackFrom (Node* from) {
		  
		}
		
	  private:
		std::unordered_map<uint32_t,std::unique_ptr<Node>> nodes;
		std::vector<std::unique_ptr<Edge> > edges;
	  };
	}
  }
}


#endif
