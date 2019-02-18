%{
#include <ostream>
#include <memory>
#include <istream>
#include "parser/tokens.hpp"
  int mylineno = 1;
  int column = 1;
  using namespace std;
  KeywordChecker kw;
  int getLineNumber () {
	return mylineno;
  }
  
  int getColumn () {
	return column;
  }
  
#define YY_USER_ACTION							\
  column += yyleng;
%}

%option noyywrap

ws      [ \t]+
alpha   [A-Za-z]
dig     [0-9]
string  ({alpha})+
num1    {dig}+
%%

{ws}    /* skip blanks and tabs */

"{" return LBRACE;
"}" return RBRACE;
":" return COLON;
"=" return EQUAL;
"," return COMMA;

{num1}  return NUMBER;

\n        { mylineno++;}

{string}   { return kw.isKeyword (yytext) ? KEYWORD : STRING;}



%%
std::unique_ptr<FlexLexer>  makeFlexer (std::istream& i) {
  auto a= std::make_unique<yyFlexLexer> ();
  a->yyrestart (i);
  return a; 
}
