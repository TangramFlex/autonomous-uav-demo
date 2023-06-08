FROM ubuntu:22.04
RUN apt update
RUN apt install -y make g++ build-essential autoconf libssl-dev libtool pkgconf
ENV LD_LIBRARY_PATH=/working/code-gen/build/libs:/working/code-gen/local_install/lib
WORKDIR /working
COPY code-gen /working/code-gen
RUN make -C code-gen
COPY MissionComputer/src /working/src
COPY MissionComputer/Makefile /working/
RUN make
CMD ["/working/mission_computer"]
