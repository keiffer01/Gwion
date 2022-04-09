# Build Stage
FROM ubuntu:20.04 as builder

## Install build dependencies.
RUN apt-get -y update && \
    apt-get -y install valgrind && \
    apt-get -y install afl++-clang && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y cmake clang

## Add source code to the build stage.
ADD . /gwion
WORKDIR /gwion

# Build Step
RUN make gwion-fuzz

# Package Stage
FROM ubuntu:20.04
COPY --from=builder /gwion/gwion-fuzz /
