#include "words/words.hpp"
#include "words/linconstraint.hpp"
#include "parser.hpp"


bool Words::Parser::parseVariablesDecl (){
  std::string t;

  if (acceptKeyword (Keywords::Variables) && accept (LBRACE)) {
	if (tryaccept(STRING,t)) {
	  for (auto c : t) 
		options->context->addVariable (c);
	  accept(RBRACE);
	  return true;
	}
  }
  return false;
}


bool Words::Parser::parseTerminalsDecl (){
  std::string t;

  if (acceptKeyword (Keywords::Terminals) && accept (LBRACE)) {
	if (tryaccept (STRING,t)) {
	  for (auto c : t) 
		options->context->addTerminal (c);
	  
	}
	accept(RBRACE);
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
	auto wb1 = options->context->makeWordBuilder (eq.lhs);	
	for (auto c : word1)
	  *wb1 << c;
	auto wb2 = options->context->makeWordBuilder (eq.rhs);
	for (auto c : word2)
	  *wb2 << c;
	eq.ctxt = options->context.get();
	return true;
  }
  return false;
}



bool Words::Parser::parseLinConstraint () {
  if (tryacceptKeyword (Keywords::LinConstraint) && accept (Tokens::COLON) && accept (Tokens::LBRACK)) {
	std::unique_ptr<Words::Constraints::LinearConstraintBuilder> builder = nullptr;
	if (tryaccept (Tokens::LEQ)) {
	  builder = Words::Constraints::makeLinConstraintBuilder (Words::Constraints::Cmp::LEq);
	}
	else if (tryaccept (Tokens::GEQ)) {
	  builder = Words::Constraints::makeLinConstraintBuilder (Words::Constraints::Cmp::GEq);
	}
	else if (tryaccept (Tokens::LT)) {
	  builder = Words::Constraints::makeLinConstraintBuilder (Words::Constraints::Cmp::Lt);
	}

	else if (accept (Tokens::GT)) {
	  builder = Words::Constraints::makeLinConstraintBuilder (Words::Constraints::Cmp::Gt);
	}
	else {
	  return false;
	}
	
	
	while (!tryaccept (Tokens::COMMA)) {
	  auto val = parsePMNumber ();
	  auto var = parseVarLength ();
	  if (var) {
		builder->addLHS (var,val);
	  }
	  else {
		builder->addLHS (val);
	  }
	}

	while (!tryaccept (Tokens::RBRACK)) {
	  auto val = parsePMNumber ();
	  auto var = parseVarLength ();
	  if (var) {
		builder->addRHS (var,val);
	  }
	  else {
		builder->addRHS (val);
	  }
	}
	options->constraints.push_back (builder->makeConstraint ());
	return true;
	
  }
  return false;
}

int64_t Words::Parser::parsePMNumber () {
  bool neg = false;
  if (tryaccept (Tokens::MINUS)) {
	neg = true;
  }
  else {
	accept (Tokens::PLUS);
  }
  std::string text;
  accept (Tokens::NUMBER,text);
  auto val = std::atoi (text.c_str());
  return neg ? -val : val;
}

Words::IEntry* Words::Parser::parseVarLength () {
  if (tryaccept (Tokens::BAR)) {
	std::string text;
	accept (Tokens::STRING,text);
	accept (Tokens::BAR);
	if (text.length () == 1) {
	  auto var = options->context->findSymbol (text[0]);
	  if (var->isVariable())
		return var;
	  else {
		*err << " Entity " <<  text << "is not a variable" << std::endl;
		return nullptr;
	  }
	}
	else {
	  *err << " ERROR for variable " <<  text<< std::endl;
	  return nullptr;
	}
  }
  return nullptr;
}

bool Words::Parser::tryaccept (Tokens t) {
  std::string s;
  return tryaccept (t,s);
}

bool Words::Parser::accept (Tokens t) {
  std::string s;
  return accept (t,s);
}

bool Words::Parser::accept (Tokens t, std::string& text) {
  if (tryaccept(t,text))
	return true;
  unexpected();
  return false;	
}

bool Words::Parser::tryaccept (Tokens t, std::string& text) {
  if (lexobject.token == t) {
	lexobject.token = static_cast<Tokens> (lexer->yylex());
	text = lexobject.text;
	lexobject.text = lexer->YYText ();
	lexobject.lineno = getLineNumber ();
	lexobject.colStart = getColumn ();
	
	return true;
  }
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
  throw ParserException ();
}

namespace Words {
  Parser_ptr makeStdParser (std::istream& is) {
	return Parser::Create (is);
  }
  
}
