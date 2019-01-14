.DELETE_ON_ERROR:
.PHONY: all bin valgrind check test docker dockerbuild clean
.DEFAULT_GOAL:=all

SRC:=src
OUT:=.out
TAGS:=.tags
BINOUT:=$(OUT)
BIN:=$(BINOUT)/catena
TESTBIN:=$(BINOUT)/catenatest
VALGRIND:=valgrind --tool=memcheck --leak-check=full

HTTPDLIBS:=$(shell pkg-config --libs libmicrohttpd)
HTTPDCFLAGS:=$(shell pkg-config --cflags libmicrohttpd)
SSLLIBS:=$(shell pkg-config --libs openssl)
SSLCFLAGS:=$(shell pkg-config --cflags openssl)
READLINELIBS:=-lreadline

CPPSRCDIRS:=$(wildcard $(SRC)/*)
CPPSRC:=$(shell find $(CPPSRCDIRS) -type f -iname \*.cpp -print)
CPPINC:=$(shell find $(CPPSRCDIRS) -type f -iname \*.h -print)

CATENASRC:=$(foreach dir, $(SRC)/catena $(SRC)/libcatena, $(filter $(dir)/%, $(CPPSRC)))
CATENAOBJ:=$(addprefix $(OUT)/, $(CATENASRC:%.cpp=%.o))
CATENAINC:=$(foreach dir, $(SRC)/catena $(SRC)/libcatena, $(filter $(dir)/%, $(CPPINC)))
CATENATESTSRC:=$(foreach dir, $(SRC)/test $(SRC)/libcatena, $(filter $(dir)/%, $(CPPSRC)))
CATENATESTOBJ:=$(addprefix $(OUT)/, $(CATENATESTSRC:%.cpp=%.o))
CATENATESTINC:=$(foreach dir, $(SRC)/test $(SRC)/libcatena, $(filter $(dir)/%, $(CPPINC)))
# libcatena is not its own binary, just a namespace; no src/obj rules necessary
LIBCATENAINC:=$(foreach dir, $(SRC)/libcatena, $(filter $(dir)/%, $(CPPINC)))

LEDGER:=genesisblock
TESTDATA:=$(wildcard test/*) $(LEDGER)
DOCKERFILE:=Dockerfile
DOCKERBUILDFILE:=doc/Dockerfile.build

WFLAGS:=-Wall -W -Werror
# clang doesn't like this
# WFLAGS+=-Wl,-z,defs
OFLAGS:=-g -O2
CPPFLAGS:=-I$(SRC)
CXXFLAGS:=-pipe -std=c++17 -pthread
EXTCPPFLAGS:=$(SSLCFLAGS) $(HTTPDCFLAGS)
CXXFLAGS:=$(CXXFLAGS) $(WFLAGS) $(OFLAGS) $(CPPFLAGS) $(EXTCPPFLAGS)

# FIXME detect this, or let it be specified
LDLIBSGTEST:=/usr/lib/x86_64-linux-gnu/libgtest.a

all: $(TAGS) $(BIN) $(TESTBIN)

$(BINOUT)/catena: $(CATENAOBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SSLLIBS) $(HTTPDLIBS) $(READLINELIBS)

$(BINOUT)/catenatest: $(CATENATESTOBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBSGTEST) $(SSLLIBS)

$(OUT)/$(SRC)/catena/%.o: $(SRC)/catena/%.cpp $(CATENAINC)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)/$(SRC)/test/%.o: $(SRC)/test/%.cpp $(CATENATESTINC)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)/$(SRC)/libcatena/%.o: $(SRC)/libcatena/%.cpp $(LIBCATENAINC)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated targets which live outside of $(OUT)
$(TAGS): $(CPPSRC) $(CPPINC)
	ctags -f $@ $^

bin: $(BIN)

check: test

test: $(TAGS) $(TESTBIN) $(TESTDATA)
	$(BINOUT)/catenatest

valgrind: $(TAGS) $(TESTBIN) $(TESTDATA)
	$(VALGRIND) $(BINOUT)/catenatest

DOCKEROUT:=$(OUT)/dockerbuilt
DOCKERINPUTS:=$(addprefix $(DOCKEROUT)/, catena catenatest hcn-ca-chain.pem genesisblock)
docker: $(DOCKERFILE) $(DOCKERINPUTS)
	docker build -f $< $(DOCKEROUT)

$(DOCKEROUT)/hcn-ca-chain.pem: doc/hcn-ca-chain.pem
	@mkdir -p $(@D)
	cp $< $@

$(DOCKEROUT)/genesisblock: genesisblock
	@mkdir -p $(@D)
	cp $< $@

dockerbuild: $(DOCKERBUILDFILE)
	docker build -f $< .
	# need to get container from above with -q
	@mkdir -p $(DOCKEROUT)/
	# will copy catena, catenatest
	docker cp $(CONTAINER):catena/.out/catena* $(DOCKEROUT)

clean:
	rm -rf $(OUT) $(TAGS)
