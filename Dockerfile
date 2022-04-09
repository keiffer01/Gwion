# Build Stage
FROM ubuntu:20.04 as builder

## Install build dependencies.
RUN apt-get -y update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y cmake clang

## Add source code to the build stage.
ADD . /gwion
WORKDIR /gwion

# Build Step
# RUN make

# Package Stage
FROM ubuntu:20.04
COPY --from=builder /gwion/gwion-fuzz /
