# Woorpje

Woorpje is a string solver for bounded word equations and regular constraints, i.e., the length of the strings substituting each variable is upper bounded by a given integer. Woorpje translates the string constraint problems into a satisfiability problem of propositional logic, and uses the award-winning SAT-solver Glucose as its SAT-Solving backend.

## Building

Tested on `gcc (Ubuntu 9.4.0-1ubuntu1~20.04) 9.4.0`.
Woorpje does not compile on clang.
Requirements: `cmake, gcc, z3, gperf, flexx`.

To build an executable that uses  SMT-LIB 2.6 as the input language, run the following steps:

```sh
mkdir build
cd build
cmake ..
ccmake . # Set ENABLE_Z3 to ON and save
cmake --build . --target woorpjeSMT
```

## Usage

After building, your can use woorpje as follows:

```sh
./woorpjeSMT --solver 1 <file>
```

## Supported Input Language

Woorpje does currently not understand the full SMT-LIB 2.6 standard.
Supported features are

- String variables (`declare-fun X () String`)
- String equality with concatenation (`str.++`)
- Regular membership predicates (`str.in_re`) with regular expressions only consisting of:
  - Concatentaion (`re.++`)
  - Union (`re.union`)
  - Kleene closure (`re.star`)
  - Empty set (`re.none`)
  - Constant strings (`str.to_re`)

Assertions containing boolean connectives (`or`, `and`, `not`) are not supported.
However, multiple assertions can be stated in a single input file.
