#include "words/words.hpp"
#include "parser/parser.hpp"


bool Words::Parser::parseVariablesDecl (){
  std::string t;

  if (acceptKeyword (Keywords::Variables) && accept (LBRACE) && accept (STRING,t),accept (RBRACE)) {
	for (auto c : t) 
	  options->context.addVariable (c);
	
	return true;
  }
  return false;
}


bool Words::Parser::parseTerminalsDecl (){
  std::string t;

  if (acceptKeyword (Keywords::Terminals) && accept (LBRACE) && accept (STRING,t),accept (RBRACE)) {
	
	for (auto c : t) 
	  options->context.addTerminal (c);
	return true;
  }
  return false;
}


bool Words::Parser::parseEquation (){
  std::string word1;
  std::string word2;
  if (tryacceptKeyword (Keywords::Equation) && accept (Tokens::COLON) && accept (STRING,word1) && accept (Tokens::EQUAL) &&  accept (STRING,word2) ) {
	options->equations.emplace_back ();
	auto& eq = options->equations.back();
	auto wb1 = options->context.makeWordBuilder (eq.lhs);	
	for (auto c : word1)
	  *wb1 << c;
	auto wb2 = options->context.makeWordBuilder (eq.rhs);
	for (auto c : word2)
	  *wb2 << c;
	return true;
  }
  return false;
}

bool Words::Parser::parseLength () {
  std::string text;
  if (acceptKeyword (Keywords::Length) && accept(Tokens::COLON) && accept (Tokens::NUMBER,text) ) {
	options->varMaxPadding = std::atoi (text.c_str());
	return true;
  }
  return false;
}

bool Words::Parser::accept (Tokens t) {
  std::string s;
  return accept (t,s);
}

bool Words::Parser::accept (Tokens t, std::string& text) {
  if (lexobject.token == t) {
	lexobject.token = static_cast<Tokens> (lexer->yylex());
	text = lexobject.text;
	lexobject.text = lexer->YYText ();
	lexobject.lineno = getLineNumber ();
	lexobject.colStart = getColumn ();
	
	return true;
  }
  unexpected();
  return false;
}
bool Words::Parser::acceptKeyword (Keywords k) {
  if (!tryacceptKeyword (k)) {
	unexpected();
	return false;
  }
  return true;
}

bool Words::Parser::tryacceptKeyword (Keywords k) {
  if (lexobject.token == KEYWORD  && k == kw.getKeyword (lexobject.text)) {
	lexobject.token = static_cast<Tokens> (lexer->yylex());
	lexobject.text  = lexer->YYText ();
	lexobject.lineno = getLineNumber ();
	lexobject.colStart = getColumn ();
	return true; 
  }
  return false;
}

void Words::Parser::unexpected () {
  *err << "On " << lexobject.lineno << "." << lexobject.colStart <<": " << " unexpected " << lexobject.text << std::endl;  
}
