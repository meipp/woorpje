#ifndef _STDLEXER__
#define _STDLEXER__
#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "tokens.hpp"

 class STDLexer : public yyFlexLexer {
  private:
	int mylineno = 1;
	int column = 1;
	
	KeywordChecker kw;
 public:	
	int getLineNumber () {
	  return mylineno;
	}
	
	int getColumn () {
	  return column;
	}

  
   virtual int yylex() override;
  };
 
#endif 
