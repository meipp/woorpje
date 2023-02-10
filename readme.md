Fix of the [woorpje](https://git.zs.informatik.uni-kiel.de/dbp/wordsolve/-/tree/spin22) string solver pinned at tag `spin22`. Fixes the linker error when compiling with `ENABLE_CVC4=on`.

# Woorpje

Woorpje is a string solver for bounded word equations and regular constraints, i.e., the length of the strings substituting each variable is upper bounded by a given integer. 
Woorpje translates the string constraint problems into a satisfiability problem of propositional logic, and uses the award-winning SAT-solver Glucose as its SAT-Solving backend.

## Running with Docker

### Building an image

To create a docker image, run

```
docker build . -t woorpje
```

### Running Woorpje in a Container

To start Woorjpe in a container, run

```
docker run -v "<file>:/instance.smt" woorpje [--simplify]
```

where `<file>` is the **absolute** path to an SMT-LIB 2.6 file. Adding `--simplify` is optional and enables preprocessing.

For example

```
docker run -v "$PWD/test/regular/url.smt:/instance.smt" woorpje --simplify
```




## Building

Tested on `gcc (Ubuntu 9.4.0-1ubuntu1~20.04) 9.4.0`.
Woorpje does not compile on clang.
Requirements: `cmake, gcc, boost, gperf, flexx, autoconf, libtool, libz-dev`.

To build an executable that uses  SMT-LIB 2.6 as the input language, run the following steps:

```sh
mkdir build
cd build
cmake ..
cmake --build . --target woorpjeSMT
```

## Usage

After building, your can use Woorpje as follows:

```sh
./woorpjeSMT --solver 1  [--simplify] <file>
```

Where `<file>` is the path to an SMT-LIB 2.6 file.
Adding `--simplify` is optional and enables preprocessing.

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
  - Loop (`re.loop`)
  - Range (`re.range`)
  - Plus (`re.+`)
  - Optional (`re.opt`)
  - Constant strings (`str.to_re`)

Assertions containing boolean connectives (`or`, `and`, and `not`) are not supported.
However, multiple assertions can be stated in a single input file.

## Known Issues

- The alphabets of terminals and variables must be disjunct. In particular, variables can't be named after terminals occurring in the constraints.
- The docker container might freeze if the mount failed (e.g. if the specified file does not exist), in this case the container needs to be killed by hand (`docker kill`)
- For some combinations of constraints, preprocessing is unstable. On error, try without `--simplify`.
