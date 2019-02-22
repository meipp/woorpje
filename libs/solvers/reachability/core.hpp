#include <unordered_map>
#include <memory>
#include <set>
#include <stdio.h>
#include <string.h>
#include <iostream>

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
		Node (std::unique_ptr<T*[]>& d,size_t s) : data(std::move(d)),size(s) {
		}
		
		const T* operator[] (size_t i) {
		  return data.get()[i];
		}
		std::unique_ptr<T*[]> data;
		size_t size;
	  };

	  ~ArrayStore () {
		for (auto s : map)
		  delete s.second;
	  }
	  
	  Node* makeNode (size_t s) {
		std::unique_ptr<T*[]> newD (new T*[s]);
		std::fill (newD.get(),newD.get()+s,nullptr);
		return insert (newD,s);
	  }

	  Node* change (Node* n,size_t pos, const T* t) {
		std::unique_ptr<T*[]> newD (new T*[n->size]);
		std::copy (n->data.get(),n->data.get()+n->size,newD.get());
		newD.get()[pos] = const_cast<T*> (t);
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
			return it->second;
		}
		auto newnode = std::make_unique<Node> (data,s);
		auto res = newnode.get();
		map.emplace(hash,newnode.release());
		return res;
	  }
	  
	  std::map<uint32_t,Node*> map;
	};

	
	struct WordPos {
	  size_t pos = 0;
	  size_t invar_pos = 0;
	};
	
	template<class T> 
	struct SearchState  {
	  auto copy () const {return std::make_unique<SearchState<T>> (*this);}
	  WordPos lhs;
	  WordPos rhs;
	  std::vector<typename ArrayStore<T>::Node*> substitutions;
	  uint32_t hash () {
		return 1;
	  }
	};

	template<class T>
	struct SearchStatePrinter {
	  SearchStatePrinter (std::ostream& os) : os (os) {}
	  void output (Words::Context& c, SearchState<T>& s,size_t bound) {
		for (size_t i = 0; i < c.nbVars (); i++) {
		  auto v = c.getVariable(i);
		  os << v->getRepr () << ": ";
		  for (size_t j = 0; j < bound; j++) {
			if ((*s.substitutions[v->getIndex ()])[j])
			  os << (*s.substitutions[v->getIndex ()])[j]->getRepr ();
			else
			  os << "*";
		  }
		  os << std::endl;
		}
	  }
	  std::ostream& os;
	};
	
	template<typename T>
	class WordWrapper {
	public:
	  WordWrapper (Words::Word& w, WordPos& pos, size_t varbound,std::vector<typename ArrayStore<T>::Node*>& s,T* eps) :
		entry(nullptr),word(w),pos(pos),varbound(varbound),subst(s) {
		if (finished ()) {
		  entry = nullptr;
		}
		else {
		  entry = word.data()[pos.pos];
		  
		}
	  }
	  
	  bool finished () { return pos.pos >= word.characters();}
	  bool isTerminal () {return entry->isTerminal ();}
	  bool isVariable () {return entry->isVariable ();}
	  const T* theEntry () {
		return entry;
	  }
	  const T* InVarTerminal () {
		return (*subst[entry->getIndex ()])[pos.invar_pos];
	}
	  bool isEpsilon () {return entry == epsilon;}
	  
	  void set (const T* n,ArrayStore<T>& store) {
		auto node = store.change(subst[entry->getIndex ()],pos.invar_pos,n);
		subst[entry->getIndex()] = node;
	  }

	  void jumpVar () {
		pos.invar_pos = 0;
		pos.pos++;
	  }

	  void increment () {
		if (isVariable()) {
		  pos.invar_pos++;
		  if (pos.invar_pos >= varbound)
			pos.pos++;
		}
		else {
		  pos.pos++;
		  pos.invar_pos = 0;
		}
	  }
		
  private:
	  T* entry;
	  Words::Word& word;
	  WordPos& pos;
	  size_t varbound;
	  std::vector<typename ArrayStore<T>::Node*>& subst;
	  const T* epsilon;
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

	  
	  void step (SearchState<IEntry>& state) {
		auto left =  makeLeft(state);
		auto right = makeRight(state);
		if (left.finished()) {
		  auto nstate = state.copy ();
		  auto nright = makeRight (*nstate);
		  if (nright.isVariable ()) {
			auto invar = nright.InVarTerminal ();
			if (invar == c.getEpsilon () || invar == nullptr) {
			  nright.jumpVar ();
			  passed.insert (nstate);
			}
		  }
		}
		
		else if (right.finished () ) {
		  auto nstate = state.copy ();
		  auto nleft = makeLeft(*nstate);
		  if (nleft.isVariable ()) {
			auto invar = nleft.InVarTerminal ();
			if (invar == c.getEpsilon () || invar == nullptr) {
			  nleft.jumpVar ();
			  passed.insert (nstate);
			}
		  }
		}

		else if (left.isTerminal () && right.isTerminal ()) {
		  if (left.theEntry () == right.theEntry ()) {
			auto nstate = state.copy ();
			auto nleft = makeLeft(*nstate);
			auto nright = makeRight(*nstate);
			nleft.increment();
			nright.increment ();
			passed.insert(nstate);
		  }
		}
		
		else if (left.isVariable () && right.isVariable ()) {
		  
		  auto leftVar = left.InVarTerminal ();
		  auto rightVar = right.InVarTerminal ();

		  if (leftVar == c.getEpsilon ()) {
			auto nstate = state.copy ();
			auto nleft = makeLeft (*nstate);
			nleft.jumpVar ();
			passed.insert (nstate);
		  }

		  else if (rightVar == c.getEpsilon ()) {
			auto nstate = state.copy ();
			auto nright = makeRight (*nstate);
			nright.jumpVar ();
			passed.insert (nstate);
		  }

		  else if (leftVar == nullptr && rightVar == nullptr) {
			for (size_t i = 0; i < c.nbTerms (); i++) {
			  auto term = c.getTerminal (i);
			  auto nstate = state.copy ();
			  auto nright = makeRight (*nstate);
			  auto nleft = makeLeft (*nstate);
			  nleft.set (term,store);
			  nright.set (term,store);
			  if (term == c.getEpsilon ()) {
				auto lstate = state.copy ();
				auto rstate = state.copy ();
				auto nnright = makeRight (*rstate);
				auto nnleft = makeLeft (*lstate);
				nnleft.set (term,store);
				nnright.set (term,store);
				nnleft.jumpVar ();
				nnright.jumpVar ();
				passed.insert(lstate);
				passed.insert(rstate);
			  }
			  else {
				nleft.increment ();
				nright.increment ();
			  }
			  passed.insert(nstate);
			}
		  }
		  
		  else if (leftVar == nullptr) {
			 auto nstate = state.copy ();
			 auto nleft = makeLeft (*nstate);
			 auto nright = makeRight (*nstate);
			 nleft.set (rightVar,store);
			 nleft.increment ();
			 nright.increment ();
			 passed.insert(nstate);
		  }

		  else if (rightVar == nullptr) {
			 auto nstate = state.copy ();
			 auto nleft = makeLeft (*nstate);
			 auto nright = makeRight (*nstate);
			 nright.set (leftVar,store);
			 nleft.increment();
			 nright.increment();
			 passed.insert(nstate);
		  }

		  else if (leftVar == rightVar) {
			auto nstate = state.copy ();
			
			auto nright = makeRight (*nstate);
			auto nleft = makeLeft (*nstate);
			nleft.increment ();
			nright.increment ();
			passed.insert(nstate);
		  }
		}

		else if (left.isVariable ()) {
		  auto nstate = state.copy ();
		  auto nleft = makeLeft (*nstate);
		  auto nright = makeRight (*nstate);
		  nleft.set (right.theEntry (),store);
		  nleft.increment ();
		  nright.increment ();
		  passed.insert (nstate);
		}
		
		else if (right.isVariable ()) {
		  auto nstate = state.copy ();
		  auto nleft = makeLeft (*nstate);
		  auto nright = makeRight (*nstate);
		  nright.set (left.theEntry (),store);
		  nright.increment ();
		  nleft.increment ();
		  passed.insert (nstate);
		}
	  }

	  bool finalState (SearchState<IEntry>& state) {
		auto left =  makeLeft(state);
		auto right = makeRight(state);
		
		return left.finished() && right.finished();
	  }
	  
	private:  
	  WordWrapper<IEntry> makeLeft (SearchState<IEntry>& state) {
		return WordWrapper<IEntry> (lhs,state.lhs,varbound,state.substitutions,c.getEpsilon ());
	  }

	  WordWrapper<IEntry> makeRight (SearchState<IEntry>& state) {
		return WordWrapper<IEntry> (rhs,state.rhs,varbound,state.substitutions,c.getEpsilon ());
	  }
	  
	  Words::Context& c;
	  Words::Word& lhs;
	  Words::Word& rhs;
	  size_t varbound;
	  ArrayStore<IEntry> store;
	  PassedWaiting& passed;
	};
	
  }
  
}
