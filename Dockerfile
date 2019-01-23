FROM debian:buster
RUN apt-get update
RUN apt-get install --assume-yes --no-install-recommends build-essential \
  nlohmann-json3-dev libreadline-dev libmicrohttpd-dev libssl-dev libgtest-dev \
  exuberant-ctags pkg-config devscripts debhelper capnproto libcapnp-dev
WORKDIR /catena

# Send source directory (minus .dockerignores) to $WORKDIR
COPY . ./

RUN make docker-debbin
