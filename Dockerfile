FROM ubuntu:latest
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get -y update && apt-get install -y --no-install-recommends apt-utils && apt-get -y upgrade
RUN apt-get install -y build-essential cmake libz-dev libboost-program-options-dev gperf flex autoconf libtool
WORKDIR woorpje
COPY libs libs/
COPY external external/
COPY package package/
COPY test test/
COPY CMakeLists.txt config.h.in woorpje2smt.cpp main.cpp ./

WORKDIR /woorpje/build

RUN cmake ..
RUN cmake  --build . --target woorpjeSMT

ENTRYPOINT ["./woorpjeSMT", "--solver", "1"]