#include <iostream>
#include <sstream>

#include "words/words.hpp"
#include "parser/parser.hpp"

int main () {
  std::string text = R"(Variables {aaA}
Terminals {bbB}
Length : 100
Equation: aaA = bbB)";

  std::stringstream str;
  str << text;

  auto parser = Words::Parser::Create (str);
  Words::Options opt;
  parser->Parse (opt);

  std::cout << opt.equation.lhs << std::endl;;
  std::cout << opt.equation.rhs << std::endl;
  std::cout << opt.wordMaxLength<<std::endl;
  std::cout << opt.varMaxPadding<<std::endl;
}
