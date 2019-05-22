#ifndef _WORDS__
#define _WORDS__

#include <algorithm>
#include <memory>
#include <vector>
#include <ostream>
#include <map>
#include <initializer_list>
#include <iostream>
#include <cassert>

#include "host/hash.hpp"
#include "words/constraints.hpp"

namespace Words {
  class Variable;
  class Terminal;
  class Sequence;
  class Context;
  class IEntry {
  public:
	IEntry (char repr,size_t i,Context* ctxt) : index(i),repr(repr),context(ctxt) {}
	virtual ~IEntry () {}
	virtual size_t getIndex ()  {return index;}
	virtual bool isVariable () const {return false;}	
	virtual bool isTerminal () const {return false;}
	virtual bool isSequence () const {return false;} 
	virtual Variable* getVariable () {return nullptr;}	
	virtual Terminal* getTerminal () {return nullptr;}
	virtual Sequence* getSequence () {return nullptr;}
	virtual const Variable* getVariable () const {return nullptr;}	
	virtual const Terminal* getTerminal () const {return nullptr;}
	virtual const Sequence* getSequence () const {return nullptr;}
	Context* getContext () const {return context;}
	virtual std::ostream& output (std::ostream& os) const  {return  os << getRepr ();}
	char getRepr () const {return repr;}
	virtual std::size_t length () const {return 1;}
  private:
	size_t index;
	char repr;
	Context* context;
  };
  
  class Variable : public IEntry {
  public:
	friend class Context;
	bool isVariable () const override {return true;}
	virtual Variable* getVariable () {return this;}
	virtual const Variable* getVariable () const {return this;}
  protected:
	Variable (char repr, size_t index,Context* ctxt) : IEntry(repr,index,ctxt) {}
  };

  class Sequence : public IEntry {
  public:
	using iterator = std::vector<IEntry*>::iterator;
	using reverse_iterator = std::vector<IEntry*>::reverse_iterator;
	using const_iterator = std::vector<IEntry*>::const_iterator;
	using const_reverse_iterator = std::vector<IEntry*>::const_reverse_iterator;

	using SeqDiff = std::vector<IEntry*>;

	friend class Context;
	bool isSequence () const override {return true;}
	virtual Sequence* getSequence () {return this;}
	virtual const Sequence* getSequence () const {return this;}
    const_iterator begin () const {return entries.begin();}
    const_iterator end () const {return entries.end();}
	const_reverse_iterator rbegin () const {return entries.rbegin();}
	const_reverse_iterator rend () const {return entries.rend();}

	iterator begin ()  {return entries.begin();}
	iterator end ()  {return entries.end();}
	
	reverse_iterator rbegin ()  {return entries.rbegin();}
	reverse_iterator rend ()  {return entries.rend();}
	
	std::ostream& output (std::ostream& os) const  override {
	  for (auto i : entries) {
		i->output (os);
	  }
	  return os;
	}
   
	size_t hash () const {
	  return Words::Hash::Hash<const IEntry*> (entries.data(), entries.size(),static_cast<uint32_t> (entries.size()));
	  
	}
	
	bool operator== (const Sequence& s) {
	  return entries == s.entries;
	}

	bool operator!= (const Sequence& s) {
	  return entries != s.entries;
	}

	size_t length () const override  {return entries.size();}

	bool isFactorOf (const Sequence& seq) {
	  if (length () > seq.length ()) {
		return false;
	  }
	  else if (length () == seq.length ()) {
		return *this == seq;
	  }
	  else {
		auto oit = seq.begin ();
		auto oend = seq.end ();
		auto mend = end();
		auto mit = begin ();
		auto it = mit;
		for (;oit != oend; ++oit) {	  
		  if (*it == *oit) {
			for (; it!=mend; ++oit,++it) {
			  if (*it != *oit) {
				break;
			  }
			}
			if (it == mend)
			  return true;
			else
			  it = mit;
		  }
		}
		return false;
	  }
	}
	
	//Check if this is prefix of oth
	bool operator< (const Sequence& oth) const  {
	  if (length () < oth.length ()) {
		auto myit = this->begin();
		auto othit = oth.begin();
		auto myend = this->end();
		for (; myit != myend; ++myit, ++othit) {
		  if (*myit != *othit)
			return false;
		}
		return true;
	  }
	  return false;
	}

	SeqDiff operator- (const Sequence& oth) const {
	  assert (oth < *this);
	  SeqDiff diff;
	  std::copy (this->begin()+oth.length(),this->end(), std::back_inserter(diff));
	  return diff;
	}

	
	bool isSuffixOf (const Sequence& oth) const  { 
	  if (length () < oth.length ()) {
		auto myit = this->rbegin();
		auto othit = oth.rbegin();
		auto myend = this->rend();
		for (; myit != myend; ++myit, ++othit) {
		  if (*myit != *othit)
			return false;
		}
		return true;
	  }
	  return false;
	}
	
	SeqDiff chopTail (const Sequence& oth) const {
	  assert (oth.isSuffixOf (*this));
	  SeqDiff diff;
	  std::copy (this->begin(),this->begin()+(length ()-oth.length()), std::back_inserter(diff));
	  return diff;
	}

	
  protected:
	Sequence (size_t index,std::vector<IEntry*> e,Context* ctxt) : IEntry('#',index,ctxt),entries(e) {}
  private:
	std::vector<IEntry*> entries;
  };

  
  
  
  class Terminal : public IEntry {
  public:
	friend class Context;
	bool isTerminal () const override {return true;}
	virtual Terminal* getTerminal () {return this;}
	virtual const Terminal* getTerminal () const {return this;}	
	virtual bool isEpsilon () const {return false;}
  protected:
	Terminal (char repr, size_t index,Context* ctxt) : IEntry(repr,index,ctxt) {}
  };  
  
  template<class Iter>
  struct SegIter {
	static void begin (Sequence& seq, Iter& iter) {
	  iter = seq.begin ();
	}
	
	static void end (Sequence& seq, Iter& iter) {
	  iter = seq.end ();
	}

	static void begin (const Sequence& seq, Iter& iter) {
	  iter = seq.begin ();
	}
	
	static void end (const Sequence& seq, Iter& iter) {
	  iter = seq.end ();
	}
	
  };
  
  template<>
  struct SegIter<Sequence::reverse_iterator> {
	static void begin (Sequence& seq, Sequence::reverse_iterator& iter) {
	  iter = seq.rbegin ();
	}
	
	
	static void end (Sequence& seq, Sequence::reverse_iterator& iter) {
	  iter = seq.rend ();
	}
	
  };

  template<>
  struct SegIter<Sequence::const_reverse_iterator> {
	static void begin (const Sequence& seq, Sequence::const_reverse_iterator& iter) {
	  iter = seq.rbegin ();
	}
	
	
	static void end (const Sequence& seq, Sequence::const_reverse_iterator& iter) {
	  iter = seq.rend ();
	}
	
  };

  
  
  class Word  {
  public:

	template<class base_iter,class innerIter>
	class Iterator {
	public:
	  using value_type = IEntry*; //almost always T
	  using reference = value_type&; //almost always T& or const T&
	  using pointer = value_type*; //almost always T* or const T*
	  
	  Iterator (const base_iter& cur, const base_iter& end) : cur(cur),end(end) {
		seqCheck ();
	  }
	  value_type operator* () {
		if (internal)
		  return *internal->it;
		return *cur;
	  }
	  bool operator== (const Iterator<base_iter,innerIter>& oth) {
		if (internal && oth.internal)
		  return internal->it == oth.internal->it && cur == oth.cur;
		if (internal || oth.internal)
		  return false;
		return cur == oth.cur;
		
	  }
	  
	  bool operator!= (const Iterator<base_iter,innerIter>& oth) {
		return !(*this == oth);
	  }
	  
	  void operator= (value_type t) {cur = t;}
	  Iterator& operator++ () {increment();return *this;}
	private:
	  void increment () {
		if (internal) {
		  ++internal->it;
		  if (internal->it != internal->end)
			return;
		}
		internal.reset();
		++cur;
		seqCheck ();
	  }

	  
	  void seqCheck ()  {
		if (cur != end && (*cur)->isSequence ()) {
		  auto seq  = (*cur)->getSequence ();
		  internal = std::make_unique<Internal> ();
		  SegIter<innerIter>::begin (*seq,internal->it);
		  SegIter<innerIter>::end (*seq,internal->end);
		  
		}
	  }
	  
	  struct Internal {
		innerIter it;
		innerIter end;
	  };
	  base_iter cur;
	  base_iter end;
	  std::unique_ptr<Internal> internal = nullptr;
	};

	
	
	using iterator = Iterator<std::vector<IEntry*>::iterator,Sequence::iterator>;
	using const_iterator = Iterator<std::vector<IEntry*>::const_iterator,Sequence::const_iterator>;
	using riterator = Iterator<std::vector<IEntry*>::reverse_iterator,Sequence::reverse_iterator>;
	using const_riterator = Iterator<std::vector<IEntry*>::const_reverse_iterator,Sequence::const_reverse_iterator>;

	using entry_iterator = std::vector<IEntry*>::iterator;
	using const_entry_iterator = std::vector<IEntry*>::const_iterator;
	using reverse_entry_iterator = std::vector<IEntry*>::reverse_iterator;
	using reverse_const_entry_iterator = std::vector<IEntry*>::const_reverse_iterator;
	
	friend class WordBuilder;
	Word () {}
	Word (std::initializer_list<IEntry*> list) : word (list) {}
	Word (std::vector<IEntry*>&& list) : word(list) {}
	~Word () {}
	
	size_t characters () const {
	  std::size_t l = 0;
	  for (auto e : word)
		l+=e->length();
	  return l;
	  //return word.size();
	}
	size_t entries () const {return word.size();}
	auto begin () const {return const_iterator(word.begin(),word.end());}
	auto end () const {return const_iterator(word.end(),word.end());}

	
    entry_iterator ebegin ()  {return word.begin();}
	entry_iterator  eend ()  {return word.end();}
	
	const_entry_iterator ebegin () const {return word.begin();}
	const_entry_iterator  eend ()  const {return word.end();}

	reverse_entry_iterator rebegin ()  {return word.rbegin();}
	reverse_entry_iterator  reend ()  {return word.rend();}
	
	reverse_const_entry_iterator rebegin () const {return word.rbegin();}
	reverse_const_entry_iterator  reend ()  const {return word.rend();}
	
	auto rbegin () const {return const_riterator(word.rbegin(),word.rend());}
	auto rend () const {return const_riterator(word.rend(),word.rend());}
	
	//auto rbegin () {return riterator(word.rbegin(),word.rend());}
	//auto rend () {return riterator(word.rend(),word.rend());}
	
	bool noVariableWord() const {
	  return word.size() == 1 &&  word.at(0)->isSequence ();
	}

	void getSequences ( std::vector<Sequence*>& seq) {
	  for (auto i : word) {
		if (i->isSequence ()) {
		  seq.push_back (i->getSequence ());
		}
	  }
	}
	
	bool substitudeVariable(IEntry* variable, const Word& to) {
	  bool replaced = false;
	  auto last_pos = word.begin();
	  Word newWord; // predict size
	  bool ranOnce = false;
	  auto it = word.begin();
	  auto end = word.end();
	  for(;it!=end;++it){
		if(*it == variable){
		  if(ranOnce){
			newWord.insert(last_pos, it);
		  }
		  newWord.insert(to.ebegin(),to.eend());
		  last_pos = it+1;
		  replaced = true;
		}
		ranOnce = true;
	  }
	  
	  if (replaced) {
		if (last_pos != word.begin() && last_pos != word.end()){
		  newWord.insert(last_pos, it);
		}
		word = newWord.word;
	  }
	  return replaced;
	}

	
	void erase_entry (entry_iterator it) {
	  word.erase (it);
	}

	
	void replace_entry (entry_iterator it,IEntry* e) {
	  std::replace(it,it+1,*it,e);
	}

	void erase_entry (reverse_entry_iterator it) {
	  auto base = it.base()-1;
	  word.erase (base);
	}

	
	void replace_entry (reverse_entry_iterator it,IEntry* e) {
	  auto base = it.base()-1;
	  std::replace(base,base+1,*it,e);
	}

	std::vector<Word> getConstSequences(){
	  std::vector<Word> words;
	  Word currentWord;
	  for (IEntry* x : word){
		if (x->isVariable()){
		  if (currentWord.characters() == 0)
			continue;
		  words.push_back(currentWord);
		  currentWord.clear();
		} else {
		  currentWord.append(x);
		}
	  }
	  return words;
	}
	
	
	
	bool operator==(Word const& rhs) const {
	  return word == rhs.word;
	}
	
	bool operator!=(Word const& rhs) const {
	  return !(*this == rhs);
	}
	
  protected:
	void append (IEntry* e) {word.push_back(e);}
	void clear () {word.clear ();}
  private:
	template <class iter>
	void insert( iter b, iter e) {
	  for (;b != e; ++b) {
		word.push_back(*b);
	  }
	}
	
	std::vector<IEntry*> word;
  };
  
  class Context;

  class WordBuilder {
  public:
	WordBuilder (Context& c,Word& w) : ctxt(c),word(w) {
	  word.clear ();
	}
	~WordBuilder ();
	WordBuilder& operator<< (char c);
	
  private:
	Context& ctxt;
	Word& word;
	std::vector<IEntry*> input;
  };
  
  class Context {
  public:
	using SeqInput = std::vector<IEntry*>;
	Context ();
	~Context ();
	IEntry* addVariable (char c);
	IEntry* addTerminal (char c);
	IEntry* addSequence (const SeqInput&);
	IEntry* findSymbol (char c) const;
	Terminal* getEpsilon ();
	std::unique_ptr<WordBuilder> makeWordBuilder (Word& w) {return std::make_unique<WordBuilder> (*this,w);}
	bool conformsToConventions () const;
	size_t nbVars () const;
	size_t nbTerms () const;
	IEntry* getTerminal (size_t) const;
	IEntry* getVariable (size_t s) const;
	const std::vector<Terminal*>& getTerminalAlphabet() const;
	const std::vector<Variable*>& getVariableAlphabet() const;
  private:
	struct Internals;
	std::shared_ptr<Internals> _internal;
  };

  struct Equation {
	Word lhs;
	Word rhs;
	Context* ctxt;
  };
  
  struct Options {
	Context context;
	std::vector<Equation> equations;
	std::vector<Constraints::Constraint_ptr> constraints;
  };
  
  using Substitution = std::map<IEntry*, Word >;

  inline std::ostream& operator<< (std::ostream& os, const IEntry& w) {
	return w.output (os);
  }
  std::ostream& operator<< (std::ostream&, const Word& w);
  
  inline std::ostream& operator<< (std::ostream& os, const Equation& w) {
	return os << w.lhs << " == " << w.rhs; 
  }

  inline std::ostream& operator<< (std::ostream& os, const Options& opt) {
	for (auto& eq : opt.equations)
	  os << eq << std::endl;
	for (auto& c : opt.constraints)
	  os << *c << std::endl;
	return os;
  }

  
  
}

#endif
