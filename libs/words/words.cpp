#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>


#include "words/words.hpp"
#include "words/exceptions.hpp"



namespace Words {

  using Variable_ptr = std::unique_ptr<Variable>;
  using Terminal_ptr = std::unique_ptr<Terminal>;

  
  struct Context::Internals {
	std::unordered_map<char,IEntry*> reprToEntry;
	std::vector<Variable*> vars;
	std::vector<Terminal*> terminals;
  };
  
  Context::Context () {
	_internal = std::make_unique<Internals> ();
	addTerminal ('_');
  }

  Context::~Context () {
	for (auto v : _internal->vars)
	  delete v;
	for (auto v : _internal->terminals)
	  delete v;
  }
  
  IEntry* Context::addVariable (char c) {
	if (_internal->reprToEntry.count (c)) {
	  return _internal->reprToEntry[c];
	}
	_internal->vars.push_back(new Variable (c,_internal->vars.size()));
	_internal->reprToEntry[c] = _internal->vars.back();
	return _internal->vars.back();
  }

  IEntry* Context::addTerminal (char c) {
	if (_internal->reprToEntry.count (c)) {
	  return _internal->reprToEntry[c];
	}
	_internal->terminals.push_back(new Terminal(c,_internal->terminals.size()));
	_internal->reprToEntry[c] = _internal->terminals.back();
	return _internal->terminals.back();
	
  }

  bool Context::conformsToConventions () const {
	for (auto v : _internal->vars) {
	  if (!std::isupper (v->getRepr ())) {
		std::cerr << v->getRepr ()<< std::endl;
		return false;
	  }
	}

	for (auto v : _internal->terminals) {
	  if (std::isupper (v->getRepr ())) {
		std::cerr << v->getRepr ()<< std::endl;
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
  IEntry* Context::findSymbol (char c) const {
	auto it = _internal->reprToEntry.find(c);
	if (it != _internal->reprToEntry.end ())
	  return it->second;
	std::stringstream  str;
	str << "Symbol '" << c <<"' not in context";
	throw WordException (str.str());
	return nullptr;
  }

  Terminal* Context::getEpsilon () {
	return _internal->terminals[0];
  }
  
  WordBuilder& WordBuilder::operator<< (char c) {
	word.append (ctxt.findSymbol (c));
	return *this;
  }

  
  std::ostream& operator<< (std::ostream& os, const Word& w) {
	for (auto c : w) {
	  os << c->getRepr ();
	}
	return os;
  }
  
  
}
