FROM debian:buster
RUN apt-get update
RUN apt-get install --assume-yes --no-install-recommends build-essential \
  nlohmann-json3-dev libreadline-dev libmicrohttpd-dev libssl-dev libgtest-dev \
  exuberant-ctags pkg-config
WORKDIR /catena

# Send requisite building materials to $WORKDIR
COPY Makefile genesisblock ./
COPY src ./src
COPY test ./test

RUN make -j all test
