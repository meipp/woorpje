#include <unordered_map>
#include <memory>
#include <set>
#include <stdio.h>
#include <string.h>

#include "words/words.hpp"

namespace Words{
  namespace Reach {
	template<typename T>
	uint32_t Hash (T*, size_t,size_t ) {
	  return 0;
	}
	
	template<typename T>
	class ArrayStore {
	public:
	  struct Node {
	  public:
		Node (std::unique_ptr<T*[]>& d,size_t s) : data(std::move(d)),size(s) {}
		const T* operator[] (size_t i) {return data.get()[i];}
		std::unique_ptr<T*[]> data;
		size_t size;
	  };

	  Node* makeNode (size_t s) {
		std::unique_ptr<T*[]> newD (new T*[s]);
		std::fill (newD.get(),newD.get()+s,nullptr);
		return insert (newD,s);
	  }

	  Node* change (Node* n,size_t pos, T*) {
		std::unique_ptr<T*[]> newD (new T*[n->size]);
		std::copy (n->data.get(),n->data.get(),newD.get());
		return insert (newD,n->size);
	  }
	  
	private :
	  Node* insert (std::unique_ptr<T*[]>& data,size_t s) {
		uint32_t hash = Hash<T*> (data.get(),s,s);
		auto it_pair = map.equal_range (hash);
		for (auto it = it_pair.first; it != it_pair.second;++it) {
		  if (it->second->size == s &&
			  !memcmp (data.get(),it->second->data.get(),s)
			  )
			return it->second.get();
		}
		auto newnode = std::make_unique<Node> (data,s);
		auto res = newnode.get();
		map.insert(std::pair<uint32_t,std::unique_ptr<Node>> (hash,std::move(newnode)));
		return res;
	  }
	  
	  std::unordered_map<uint32_t,std::unique_ptr<Node>> map;
	};

	
	struct WordPos {
	  size_t pos = 0;
	  size_t invar_pos = 0;
	};
	
	template<class T> 
	struct SearchState  {
	  WordPos lhs;
	  WordPos rhs;
	  std::vector<typename ArrayStore<T>::Node*> substitutions;
	  uint32_t hash () {
		return 1;
	  }
	};

	
	class PassedWaiting {
	public:
	  void insert (std::unique_ptr<SearchState<Words::IEntry>>& var) {
		auto hash = var->hash ();
		if (!seen.count(hash)) {
		  waiting.push_back(std::move(var));
		}
	  }

	  bool hasWaiting () {return waiting.size();}
	  
	  std::unique_ptr<SearchState<IEntry>> pull () {
		auto res = std::move(waiting.back());
		waiting.pop_back();
		return res;
	  }
	  
	private:
	  std::set<uint32_t> seen;
	  std::vector<std::unique_ptr<SearchState<IEntry>>> waiting;
	};

	class SuccGenerator  {
	public:
	  SuccGenerator  (Words::Context& c,Words::Word& lhs,
					  Words::Word& rhs,size_t varbound,
					  PassedWaiting& passed
					  ) : c(c),lhs(lhs),rhs(rhs),varbound(varbound),passed(passed) {}


	  void makeInitial () {
		auto state =std::make_unique<SearchState<IEntry>> ();
		state->substitutions.reserve (c.nbVars());
		auto node =store.makeNode (varbound);
		for (size_t i = 0; i < c.nbVars();i++) {
		  state->substitutions.push_back (node);
		}
		passed.insert(state);
	  }

	  bool satisfying (SearchState<IEntry>& state) {
		if (state.lhs.pos > lhs.characters () &&
			state.rhs.pos > rhs.characters ()
			) {
		  return true;
		}
	  }
	  
	  void step (const SearchState<IEntry>& state) {
		auto lentry = lhs.data()[state.lhs.pos];
		auto rentry = rhs.data()[state.rhs.pos];
		if (lentry->isTerminal () && rentry->isTerminal () ) {
		  if (lentry == rentry) {
			auto n = std::make_unique<SearchState<IEntry>> (state);
			n->lhs.pos++;
			n->rhs.pos++;
			passed.insert(n);
		  }
		}
		else if (lentry->isVariable() && rentry->isVariable ()) {
		 
		  
		}
	  }
	  
	private:

	  Words::Context& c;
	  Words::Word& lhs;
	  Words::Word& rhs;
	  size_t varbound;
	  ArrayStore<IEntry> store;
	  PassedWaiting& passed;
	};
	
  }
  
}
