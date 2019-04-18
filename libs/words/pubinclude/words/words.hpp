#ifndef _WORDS__
#define _WORDS__

#include <memory>
#include <vector>
#include <ostream>
#include <map>
#include <initializer_list>
#include <iostream>

#include "words/constraints.hpp"

namespace Words {
  class Variable;
  class Terminal;	
  class IEntry {
  public:
	IEntry (char repr,size_t i) : index(i),repr(repr) {}
	virtual ~IEntry () {}
	virtual size_t getIndex ()  {return index;}
	virtual bool isVariable () const {return false;}	
	virtual bool isTerminal () const {return false;}
	virtual Variable* getVariable () {return nullptr;}	
	virtual Terminal* getTerminal () {return nullptr;}
	virtual const Variable* getVariable () const {return nullptr;}	
	virtual const Terminal* getTerminal () const {return nullptr;}
	char getRepr () const {return repr;}
  private:
	size_t index;
	char repr;
  };
  
  class Variable : public IEntry {
  public:
	friend class Context;
	bool isVariable () const override {return true;}
	virtual Variable* getVariable () {return this;}
	virtual const Variable* getVariable () const {return this;}
  protected:
	Variable (char repr, size_t index) : IEntry(repr,index) {}
  };

  
  class Terminal : public IEntry {
  public:
	friend class Context;
	bool isTerminal () const override {return true;}
	virtual Terminal* getTerminal () {return this;}
	virtual const Terminal* getTerminal () const {return this;}	
	virtual bool isEpsilon () const {return false;}
  protected:
	Terminal (char repr, size_t index) : IEntry(repr,index) {}
  };


  class Word  {
  public:
	friend class WordBuilder;
	Word () {}
	Word (std::initializer_list<IEntry*> list) : word (list) {}
	Word (std::vector<IEntry*>&& list) : word(list) {}
	~Word () {}
	size_t characters () const {return word.size();}
	auto begin () const {return word.begin();}
	auto end () const {return word.end();}
	auto rbegin () const {return word.rbegin();}
	auto rend () const {return word.rend();}
	IEntry** data ()  {return word.data ();}
	auto get(size_t index) {return word[index];}
	bool noVariableWord() const {
		for (auto a : word){
			if (a->isVariable())
				return false;
		}
		return true;
	  }
	bool substitudeVariable(IEntry*& variable, Word& to) {
	    bool replaced = false;
	    auto last_pos = word.begin();
	    std::vector<IEntry*> newWord; // predict size
	    bool ranOnce = false;
	    auto it = word.begin();
	    auto end = word.end();
	    for(;it!=end;++it){

	    	if(*it == variable){
	    		if(ranOnce){
	    			newWord.insert(newWord.end(), last_pos, it-1);
	    		}
	    		newWord.insert(newWord.end(),to.begin(),to.end());
	    		last_pos = it;
	    		replaced = true;
	    	}
	    	ranOnce = true;
	    }

	    if (replaced) {
	    	word = newWord;
	    }
	    return replaced;
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
	std::vector<IEntry*> word;
  };
  
  class Context;

  class WordBuilder {
  public:
	WordBuilder (Context& c,Word& w) : ctxt(c),word(w) {
	  word.clear ();
	}
	WordBuilder& operator<< (char c);
  private:
	Context& ctxt;
	Word& word;
  };
  
  class Context {
  public:
	Context ();
	~Context ();
	IEntry* addVariable (char c);
	IEntry* addTerminal (char c);
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
	std::unique_ptr<Internals> _internal;
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
  
  using Substitution = std::map<IEntry*, std::vector<IEntry*> >;

  std::ostream& operator<< (std::ostream&, const Word& w);
  
  }

#endif
