FROM debian:buster
RUN apt-get update
RUN apt-get install --assume-yes --no-install-recommends \
  libreadline7 libmicrohttpd12 libssl1.1 libgtest-dev

# Send requisite building materials to $WORKDIR
COPY catena catenatest /usr/bin/
COPY genesisblock /usr/share/catena/
COPY hcn-ca-chain.pem /usr/share/catena/
