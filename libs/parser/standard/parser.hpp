#include <memory>
#include <istream>
#include <FlexLexer.h>
#include <string>
#include <ostream>

#include "parser/parsing.hpp"
#include "words/words.hpp"
#include "words/exceptions.hpp"
#include "solvers/solvers.hpp"
#include "tokens.hpp"
#include "lexer.hpp"


std::unique_ptr<STDLexer> makeSTDFlexer (std::istream& is);

int getLineNumber ();
int getColumn ();

class JGenerator : public Words::JobGenerator  {
  public:
  JGenerator ()  {
	job = std::make_unique<Words::Job> ();}
  std::unique_ptr<Words::Job> newJob ()  {
	return std::move(job);
  }
  std::unique_ptr<Words::Job> job = nullptr;
};


namespace Words {
  class ParserException : public WordException {
  public:
	ParserException () : WordException ("Parser  Error") {}
  };
  
  class Parser : public ParserInterface {
  public:
	static std::unique_ptr<Parser> Create (std::istream& is)  {
	  return std::unique_ptr<Parser> (new Parser (makeSTDFlexer (is)));
	}
	
	std::unique_ptr<JobGenerator> Parse (std::ostream& err) {
	  auto gen = std::make_unique<JGenerator> (); 
	  try {
		this->err = &err; 
		options = &gen->job->options;
		options->context = std::make_shared<Words::Context> ();
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
			  gen->job->solver = Solvers::makeSolver<Solvers::Types::SatEncoding> (static_cast<size_t> (std::atoi (text.c_str ())));
		  }
		  if (tryacceptKeyword (SatGlucoseOld)) {
			std::string text;
			if (accept(Tokens::LPARAN) && 
				accept(Tokens::NUMBER,text) && 
				accept(Tokens::RPARAN)
				)
			  
			  gen->job->solver = Solvers::makeSolver<Solvers::Types::SatEncodingOld> (static_cast<size_t> (std::atoi (text.c_str ())));
		  }
		  
		  /*if (tryacceptKeyword (Reachability)) {
			std::string text;
			if (accept(Tokens::LPARAN) && 
			accept(Tokens::NUMBER,text) && 
			accept(Tokens::RPARAN)
			)
			
			return Solvers::makeSolver<Solvers::Types::Reachability> (static_cast<size_t> (std::atoi (text.c_str ())));
			}*/
		  if (tryacceptKeyword (SMT)) {
			gen->job->solver = Solvers::makeSolver<Solvers::Types::PureSMT> ();
		  }
		  if (tryacceptKeyword (Levis)) {
			gen->job->solver = Solvers::makeSolver<Solvers::Types::Levis> ();
		  }
		}

	  }
	  catch (ParserException& p) {
		gen->job = nullptr;
	  }
	  
	  return gen;
	}
	
  private:
	Parser (std::unique_ptr<STDLexer>&& lex ) :lexer(std::move(lex)) {
	  lexobject.token = static_cast<Tokens>(lexer->yylex());
	  lexobject.text = lexer->YYText ();
	  lexobject.lineno = lexer->getLineNumber();
	  lexobject.colStart = lexer->getColumn ();
	  
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
    std::unique_ptr<STDLexer> lexer;
	Words::Options* options = nullptr;
	std::ostream* err;
  };
}
