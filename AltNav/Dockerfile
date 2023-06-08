FROM tangramflex/pro-builder:latest AS build
ENV LD_LIBRARY_PATH=/root/code-gen/build/libs:/root/code-gen/local_install/lib
WORKDIR /root
COPY code-gen /root/code-gen
RUN make -C code-gen
COPY src /root/src
COPY Makefile /root
RUN make
CMD ["alt_nav"]
