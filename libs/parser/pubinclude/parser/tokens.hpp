#ifndef _TOKENS__
#define _TOKENS__
#include <string>


#define LISTOFTOKENS								\
  X(FIRST)											\
  X(STRING)											\
  X(LBRACE)											\
  X(RBRACE)											\
  X(COLON)											\
  X(EQUAL)											\
  X(NUMBER)											\
  X(KEYWORD)										\
  X(COMMA)											\
  X(LBRACK)										\
  X(RBRACK)											\
  X(PLUS)											\
  X(MINUS)											\
  X(BAR)											\
  X(LEQ)											\
  X(GEQ)											\
  X(LT)											\
  X(GT)											\
  X(LAST) 

enum  Tokens {
#define X(x) x,
LISTOFTOKENS
#undef X
};

enum  Keywords{
					Variables,
					Terminals,
					Equation,
					Length,
					LinConstraint,
					SatGlucose,
					SatGlucoseOld
};



struct LexObject {
  LexObject (std::string s,Tokens tok, size_t lineno, size_t colStart) : text(s),token(tok),lineno(lineno),colStart(colStart) {} 
  LexObject () {}
  std::string text;
  Tokens token;
  size_t lineno;
  size_t colStart;
};

struct KeywordInfo {
  const char* name; Tokens token; Keywords keyword;
};

class KeywordChecker {
public:
  bool isKeyword (std::string name);
  Keywords getKeyword (std::string name);
  const char* keywordText (size_t);
};

#endif
