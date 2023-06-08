FROM ubuntu:22.04
RUN apt update && apt install -y make g++ build-essential autoconf libssl-dev libtool pkgconf
ENV LD_LIBRARY_PATH=/root/code-gen/build/libs:/root/code-gen/local_install/lib
WORKDIR /root
COPY code-gen /root/code-gen
RUN make -C code-gen
COPY src /root/src
COPY Makefile /root
RUN make
CMD ["alt_nav"]
