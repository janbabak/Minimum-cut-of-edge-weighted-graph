FROM ubuntu:22.04

RUN apt update && DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata && apt upgrade -y
RUN apt install -y build-essential git cmake gdb clang make g++ libomp-dev valgrind mpich

WORKDIR /work