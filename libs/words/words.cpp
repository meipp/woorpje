#include <cassert>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>


#include "words/words.hpp"
#include "words/exceptions.hpp"



namespace Words {

  
  struct Context::Internals {
	~Internals () {
	  for (auto v : vars)
		delete v;
	  for (auto v : terminals)
		delete v;
	  for (auto v : sequences)
		delete v;
	}

    IEntry* addVariable (const std::string& c,Context* ctxt) {
      if (reprToEntry.count (c)) {
		IEntry* e = reprToEntry[c];
		if (!e->getVariable ()) {
		  throw WordException ("Adding previously defined terminal as variable");
		}
		return reprToEntry[c];
      }
      vars.push_back(new Variable (c,vars.size(),ctxt));
      reprToEntry[c] = vars.back();
      return vars.back();
    }

    IEntry* addTerminal (char c,Context* ctxt, bool epsilon) {
      if (reprToEntry.count (std::string(1,c))) {
		IEntry* e = reprToEntry[std::string(1,c)];
		if (!e->getTerminal ()) {
		  throw WordException ("Adding previously defined variable as terminal");
		}
		if (!epsilon && reprToEntry[std::string(1,c)]->getTerminal()->isEpsilon ()) {
		  throw UsingEpsilonAsNonEpsilon ();
		}
		return reprToEntry[std::string(1,c)];
      }
      terminals.push_back(new Terminal(c,terminals.size(),ctxt,epsilon));
      reprToEntry[std::string(1,c)] = terminals.back();
      return terminals.back();
	  
    }

    std::unordered_map<std::string,IEntry*> reprToEntry;
    std::unordered_map<size_t,Sequence*> hashToSequence;
    std::vector<Variable*> vars;
    std::vector<Terminal*> terminals;
    std::vector<Sequence*> sequences;

  };
  
  Context::Context () {
	_internal = std::make_shared<Internals> ();
	_internal->addTerminal ('_',this,true);
  }

  Context::~Context () {
	
  }

  IEntry* Context::addVariable (const std::string& c) {
    return _internal->addVariable (c,this);
  }

  IEntry* Context::addTerminal (char c) {
    return _internal->addTerminal (c,this,false);
  }

  IEntry* Context::addSequence (const Context::SeqInput& s) {
	std::unique_ptr<Sequence> nentry (new Sequence (_internal->sequences.size(),s,this));
	auto hash = nentry->hash ();
	auto it = _internal->hashToSequence.find (hash);
	if (it != _internal->hashToSequence.end ()) {
	  assert (*nentry == *it->second && "Hash Collision");
	  if (*nentry != *it->second) {
		throw Words::WordException ("Hash Collision while adding Sequence");
	  }
	  return it->second;
	}
	Sequence* seq = nentry.release ();
	_internal->sequences.push_back (seq);
	_internal->hashToSequence.insert(std::make_pair (hash,seq));
	return _internal->sequences.back();
  }
  
  bool Context::conformsToConventions () const {
    
    for (auto v : _internal->vars) {
	  
	  if (v->getName().size()!=1 ||
		  !std::isupper(v->getName()[0])) { 
		return false;
      } 
    }
    for (auto v : _internal->terminals) {
	  if (v->isEpsilon ())
		continue;
	  if (v->getName().size()!=1 ||
		  std::isupper(v->getName()[0])) {
		return false;
      }  
    }
    return true;
  }

  size_t Context::nbVars () const {
	return _internal->vars.size();
  }

  size_t Context::nbTerms () const {
	return _internal->terminals.size();
  }

  IEntry* Context::getTerminal (size_t s) const {
	return _internal->terminals[s];
  }
  
  IEntry* Context::getVariable (size_t s) const {
	return _internal->vars[s];
  }
  const std::vector<Terminal*>& Context::getTerminalAlphabet() const {
	return _internal->terminals;
  }
  const std::vector<Variable*>& Context::getVariableAlphabet() const {
	return _internal->vars;
  }
  IEntry* Context::findSymbol (const std::string& c) const {
	auto it = _internal->reprToEntry.find(c);
	if (it != _internal->reprToEntry.end ())
	  return it->second;
	std::stringstream  str;
	str << "Symbol '" << c <<"' not in context";
	throw WordException (str.str());
	return nullptr;
  }

  Terminal* Context::getEpsilon () const {
	return _internal->terminals[0];
  }
  
  WordBuilder::~WordBuilder () {
	flush ();
  }
  
  WordBuilder& WordBuilder::operator<< (char c) {
    return operator<< (std::string(1,c));
  }

  WordBuilder& WordBuilder::operator<< (const std::string& c) {
    auto entry = ctxt.findSymbol (c);
    if (entry->isTerminal ()) {
      input.push_back (entry);
    }
    if (entry->isVariable ()) {
      if (input.size()) {
	auto seq = ctxt.addSequence (input);
	word.append (seq);
      }
      word.append (entry);
      input.clear ();
    }
    return *this;
  }
  
  void WordBuilder::flush  ()  {
    if (input.size ()) {
      auto seq = ctxt.addSequence (input);
      word.append (seq);
    }
    input.clear ();
  }
  
  std::ostream& operator<< (std::ostream& os, const Word& w) {
	for (auto it = w.ebegin(); it != w.eend();++it) {
	  os << **it;
	  os << ' ';
	}
	return os;
  }

  std::ostream& operator<< (std::ostream& os, const Substitution& sub) {
	os << "{ ";
	for (auto& s: sub)
	  os << *s.first << " -> "  << s.second << " ";
	return os << " }";
  }
  
}
