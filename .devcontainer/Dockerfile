FROM ubuntu:22.04
RUN apt update && apt install -y \
    cmake \
    make \
    g++ \
    gcc \
    git \
    python3 \
    python3-pip \
    gdb \
    pkg-config \
    clangd \
    ninja-build

RUN pip3 install scikit-build-core==0.2.2
    
WORKDIR /opt
RUN git clone https://github.com/open62541/open62541
WORKDIR /opt/open62541
RUN git fetch --all --tags
RUN git checkout tags/v1.3.10 -b v1.3.10-branch

RUN git submodule update --init --recursive
WORKDIR /opt/open62541/build
RUN cmake -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DUA_NAMESPACE_ZERO=FULL \
    -DUA_ENABLE_JSON_ENCODING=ON \
    -DUA_MULTITHREADING=200 \
    .. && \
    make -j && \
    make install

WORKDIR /opt
RUN git clone https://github.com/FraunhoferIOSB/swap-it-open62541-server-template
WORKDIR /opt/swap-it-open62541-server-template/build
RUN cmake .. && \
    make -j && \
    make install

ENV LD_LIBRARY_PATH=/usr/local/lib

EXPOSE 4840