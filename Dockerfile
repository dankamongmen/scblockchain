# This one builds the Alpine binary package...
FROM alpine:edge as builder
RUN apk add --update alpine-sdk libmicrohttpd-dev openssl-dev ctags \
  ncurses-dev readline-dev
WORKDIR /catena
# Send source directory (minus .dockerignores) to $WORKDIR
COPY . ./
RUN make docker-apk

# This one builds a production image, and installs the binary package
FROM alpine:latest
RUN apt-get update
RUN apt-get install --assume-yes --no-install-recommends \
  libreadline7 libmicrohttpd12 libssl1.1 libcapnp-0.7.0 libgtest-dev
EXPOSE 8080 40404
# Send requisite building materials to /tmp from the previous image
COPY --from=builder /catena-* /tmp/
COPY --from=builder /catena_* /tmp/
RUN dpkg -i /tmp/*deb
