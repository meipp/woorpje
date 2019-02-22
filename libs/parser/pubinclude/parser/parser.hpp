#include <memory>
#include <istream>
#include <FlexLexer.h>
#include <string>
#include <ostream>

#include "words/words.hpp"
#include "solvers/solvers.hpp"
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
	
	Solvers::Solver_ptr Parse (Words::Options& opt,std::ostream& err) {
	  this->err = &err; 
	  options = &opt;
	  if (parseVariablesDecl () && 
		  parseTerminalsDecl ()
		  ) {
		while(parseEquation ()) {}
		while(parseLinConstraint ()) {}
		if (tryacceptKeyword (SatGlucose)) {
		  std::string text;
		  if (accept(Tokens::LPARAN) && 
			  accept(Tokens::NUMBER,text) && 
			  accept(Tokens::RPARAN)
			  )
			return Solvers::makeSolver<Solvers::Types::SatEncoding> (static_cast<size_t> (std::atoi (text.c_str ())));
		}
		if (tryacceptKeyword (SatGlucoseOld)) {
		  std::string text;
		  if (accept(Tokens::LPARAN) && 
			  accept(Tokens::NUMBER,text) && 
			  accept(Tokens::RPARAN)
			  )
			
			return Solvers::makeSolver<Solvers::Types::SatEncodingOld> (static_cast<size_t> (std::atoi (text.c_str ())));
		}
		
	  }
	  return nullptr;
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
	bool parseLinConstraint ();
	int64_t parsePMNumber ();
	Words::IEntry* parseVarLength ();
	
	
	bool accept (Tokens t);
	bool tryaccept (Tokens t);
	bool tryaccept (Tokens, std::string&);
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
