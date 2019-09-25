%{
#include <ostream>
#include <memory>
#include <istream>
#include "tokens.hpp"
#include "lexer.hpp"

  
  using namespace std;
  
#define YY_USER_ACTION							\
  column += yyleng;
%}

%option noyywrap
%option yyclass="STDLexer"


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
std::unique_ptr<STDLexer>  makeSTDFlexer (std::istream& i) {
  auto a= std::make_unique<STDLexer> ();
  a->yyrestart (i);
  return a; 
}
