#pragma once
#include "core/Solver.h"
#include "nfa.h"
#include "regencoding.h"
#include "words/regconstraints.hpp"
#include "words/words.hpp"

namespace RegularEncoding {


class FilledPos {
   public:
    FilledPos(int tIndex) : tIndex(tIndex), isTerm(true){};

    FilledPos(std::pair<int, int> vIndex) : vIndex(std::move(vIndex)), isTerm(false){};

    bool isTerminal() { return isTerm; }

    bool isVariable() { return !isTerm; }

    int getTerminalIndex() { return tIndex; }

    std::pair<int, int> getVarIndex() { return vIndex; }

   private:
    bool isTerm;
    int tIndex = -1;
    std::pair<int, int> vIndex = std::make_pair(-1, -1);
};

int numTerminals(std::vector<FilledPos> &pat, int from = 0, int to = -1);

class Encoder {
   public:
    Encoder(Words::RegularConstraints::RegConstraint constraint, Words::Context ctx, Glucose::Solver &solver, int sigmaSize, std::map<Words::Variable *, int> *vIndices, std::map<int, int> *maxPadding,
            std::map<Words::Terminal *, int> *tIndices, std::map<std::pair<std::pair<int, int>, int>, Glucose::Var> *variableVars, std::map<std::pair<int, int>, Glucose::Var> *constantsVars,
            std::map<int, Words::Terminal *> &index2t)
        : constraint(std::move(constraint)),
          ctx(ctx),
          vIndices(vIndices),
          tIndices(tIndices),
          index2t(index2t),
          maxPadding(maxPadding),
          variableVars(variableVars),
          constantsVars(constantsVars),
          solver(solver),
          sigmaSize(sigmaSize),
          ffalse(PropositionalLogic::PLFormula::lit(0)),
          ftrue(PropositionalLogic::PLFormula::lit(0)) {
        Glucose::Var falseVar = solver.newVar();
        ffalse = PropositionalLogic::PLFormula::land(std::vector<PropositionalLogic::PLFormula>{PropositionalLogic::PLFormula::lit(falseVar), PropositionalLogic::PLFormula::lit(-falseVar)});

        Glucose::Var trueVar = solver.newVar();
        ftrue = PropositionalLogic::PLFormula::lor(std::vector<PropositionalLogic::PLFormula>{PropositionalLogic::PLFormula::lit(trueVar), PropositionalLogic::PLFormula::lit(-trueVar)});
    };

    virtual ~Encoder(){};

    virtual std::set<std::set<int>> encode(){};

    std::vector<FilledPos> filledPattern(const Words::Word &);

   protected:
    Words::RegularConstraints::RegConstraint constraint;
    Words::Context ctx;
    std::map<Words::Variable *, int> *vIndices;
    std::map<Words::Terminal *, int> *tIndices;
    std::map<int, int> *maxPadding;
    std::map<std::pair<std::pair<int, int>, int>, Glucose::Var> *variableVars;
    std::map<std::pair<int, int>, Glucose::Var> *constantsVars;
    Glucose::Solver &solver;
    int sigmaSize;
    std::map<int, Words::Terminal *> &index2t;

    PropositionalLogic::PLFormula ffalse;
    PropositionalLogic::PLFormula ftrue;
};

class InductiveEncoder : public Encoder {
   public:
    InductiveEncoder(Words::RegularConstraints::RegConstraint constraint, Words::Context ctx, Glucose::Solver &solver, int sigmaSize, std::map<Words::Variable *, int> *vIndices,
                     std::map<int, int> *maxPadding, std::map<Words::Terminal *, int> *tIndices, std::map<std::pair<std::pair<int, int>, int>, Glucose::Var> *variableVars,
                     std::map<std::pair<int, int>, Glucose::Var> *constantsVars, std::map<int, Words::Terminal *> &index2t, InductiveProfiler &profiler)
        : Encoder(constraint, ctx, solver, sigmaSize, vIndices, maxPadding, tIndices, variableVars, constantsVars, index2t),
          profiler(profiler){

          };

    std::set<std::set<int>> encode();

   private:
    PropositionalLogic::PLFormula doEncode(const std::vector<FilledPos> &, const std::shared_ptr<Words::RegularConstraints::RegNode> &expression);

    PropositionalLogic::PLFormula encodeWord(std::vector<FilledPos> filledPat, std::vector<int> expressionIdx);

    PropositionalLogic::PLFormula encodeUnion(std::vector<FilledPos> filledPat, const std::shared_ptr<Words::RegularConstraints::RegOperation> &expression);

    PropositionalLogic::PLFormula encodeConcat(std::vector<FilledPos> filledPat, const std::shared_ptr<Words::RegularConstraints::RegOperation> &expression);

    PropositionalLogic::PLFormula encodeStar(std::vector<FilledPos> filledPat, const std::shared_ptr<Words::RegularConstraints::RegOperation> &expression);

    PropositionalLogic::PLFormula encodeNone(const std::vector<FilledPos> &filledPat, const std::shared_ptr<Words::RegularConstraints::RegEmpty> &empty);

    // std::unordered_map<size_t, PropositionalLogic::PLFormula*> ccache;
    InductiveProfiler &profiler;
};

class AutomatonEncoder : public Encoder {
   public:
    AutomatonEncoder(Words::RegularConstraints::RegConstraint constraint, Words::Context ctx, Glucose::Solver &solver, int sigmaSize, std::map<Words::Variable *, int> *vIndices,
                     std::map<int, int> *maxPadding, std::map<Words::Terminal *, int> *tIndices, std::map<std::pair<std::pair<int, int>, int>, Glucose::Var> *variableVars,
                     std::map<std::pair<int, int>, Glucose::Var> *constantsVars, std::map<int, Words::Terminal *> &index2t, AutomatonProfiler &profiler)
        : Encoder(constraint, ctx, solver, sigmaSize, vIndices, maxPadding, tIndices, variableVars, constantsVars, index2t),
          profiler(profiler){

          };

    std::set<std::set<int>> encode();

   private:
    PropositionalLogic::PLFormula encodeTransition(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat);

    PropositionalLogic::PLFormula encodePredecessor(Automaton::NFA &nfa, std::vector<FilledPos> filledPat);

    PropositionalLogic::PLFormula encodeFinal(Automaton::NFA &nfa, const std::vector<FilledPos> &filledPat);

    PropositionalLogic::PLFormula encodeInitial(Automaton::NFA &nfa);

    PropositionalLogic::PLFormula encodePredNew(Automaton::NFA &Mxi, std::vector<FilledPos> filledPat, int q, int i, std::set<std::pair<int, int>> &,
                                                std::map<int, std::set<std::pair<Words::Terminal *, int>>> &);

    Automaton::NFA filledAutomaton(Automaton::NFA &nfa);

    std::map<std::pair<int, int>, int> stateVars{};

    std::unordered_map<int, std::shared_ptr<LengthAbstraction::ArithmeticProgressions>> satewiseLengthAbstraction{};

    std::map<int, std::shared_ptr<std::set<int>>> reachable;

    AutomatonProfiler &profiler;
};

}  // namespace RegularEncoding