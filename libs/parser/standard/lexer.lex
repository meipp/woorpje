%{
#include <ostream>
#include <memory>
#include <istream>
#include "tokens.hpp"
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
"(" return LPARAN;
")" return RPARAN;
":" return COLON;
"=" return EQUAL;
"," return COMMA;
"[" return LBRACK;
"]" return RBRACK;
"+" return PLUS;
"-" return MINUS;
"|" return BAR;
"<=" return LEQ;
"<" return LT;
">=" return GEQ;
">" return GT;


{num1}  return NUMBER;

\n        { mylineno++;column = 0;}

{string}   { return kw.isKeyword (yytext) ? KEYWORD : STRING;}



%%
std::unique_ptr<FlexLexer>  makeFlexer (std::istream& i) {
  auto a= std::make_unique<yyFlexLexer> ();
  a->yyrestart (i);
  return a; 
}
