# This one builds the Debian source and binary packages...
FROM debian:buster as builder
RUN apt-get update
RUN apt-get install --assume-yes --no-install-recommends build-essential \
  nlohmann-json3-dev libreadline-dev libmicrohttpd-dev libssl-dev libgtest-dev \
  exuberant-ctags pkg-config devscripts debhelper capnproto libcapnp-dev
WORKDIR /catena
# Send source directory (minus .dockerignores) to $WORKDIR
COPY . ./
RUN make docker-debbin

# This one builds a production image, and installs the binary package
FROM debian:buster
RUN apt-get update
RUN apt-get install --assume-yes --no-install-recommends \
  libreadline7 libmicrohttpd12 libssl1.1 libcapnp-0.7.0 libgtest-dev
EXPOSE 8080 40404
# Send requisite building materials to /tmp from the previous image
COPY --from=builder /catena-* /tmp/
COPY --from=builder /catena_* /tmp/
RUN dpkg -i /tmp/*deb
