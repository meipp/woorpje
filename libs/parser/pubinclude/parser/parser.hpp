#include <memory>
#include <istream>
#include <FlexLexer.h>
#include <string>
#include <ostream>

#include "words/words.hpp"
#include "parser/tokens.hpp"

std::unique_ptr<FlexLexer> makeFlexer (std::istream& is);

int getLineNumber ();
int getColumn ();



namespace Words {

  
  class Parser {
  public:
	static std::unique_ptr<Parser> Create (std::istream& is)  {
	  return std::unique_ptr<Parser> (new Parser (makeFlexer (is)));
	}
	
	bool Parse (Words::Options& opt,std::ostream& err) {
	  this->err = &err; 
	  options = &opt;
	  if (parseVariablesDecl () && 
		  parseTerminalsDecl () && 
		  parseLength ()) {
		while(parseEquation ()) {}
		return acceptKeyword (SatGlucose);
	  }
	  return false;
	}
	
  private:
	Parser (std::unique_ptr<FlexLexer>&& lex ) :lexer(std::move(lex)) {
	  lexobject.token = static_cast<Tokens>(lexer->yylex());
	  lexobject.text = lexer->YYText ();
	  lexobject.lineno = getLineNumber();
	  lexobject.colStart = getColumn ();
	  
	}

	bool parseVariablesDecl ();
	bool parseTerminalsDecl ();
	bool parseEquation ();
	bool parseLength ();
	
	bool accept (Tokens t);
	bool accept (Tokens, std::string&);
	bool acceptKeyword (Keywords);
	bool tryacceptKeyword (Keywords);
	void unexpected ();
	KeywordChecker kw;
	LexObject lexobject;
	std::unique_ptr<FlexLexer> lexer;
	Words::Options* options = nullptr;
	std::ostream* err;
  };
}
