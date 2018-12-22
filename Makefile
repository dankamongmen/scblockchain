.DELETE_ON_ERROR:
.PHONY: all bin valgrind check test clean
.DEFAULT_GOAL:=all

SRC:=src
EXTSRC:=ext
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

CPPSRCDIRS:=$(wildcard $(SRC)/* $(EXTSRC))
CPPSRC:=$(shell find $(CPPSRCDIRS) -type f -iname \*.cpp -print)
CPPINC:=$(shell find $(CPPSRCDIRS) -type f -iname \*.h -print)

CATENASRC:=$(foreach dir, src/catena src/libcatena, $(filter $(dir)/%, $(CPPSRC)))
CATENAOBJ:=$(addprefix $(OUT)/,$(CATENASRC:%.cpp=%.o))
CATENATESTSRC:=$(foreach dir, src/test src/libcatena, $(filter $(dir)/%, $(CPPSRC)))
CATENATESTOBJ:=$(addprefix $(OUT)/,$(CATENATESTSRC:%.cpp=%.o))

LEDGER:=genesisblock
TESTDATA:=test/genesisblock-test $(LEDGER)

WFLAGS:=-Wall -W -Werror
# clang doesn't like this
# WFLAGS+=-Wl,-z,defs
OFLAGS:=-g -O2
CPPFLAGS:=-I$(SRC)
CXXFLAGS:=-pipe -std=c++14 -pthread
EXTCPPFLAGS:=$(SSLCFLAGS) $(HTTPDCFLAGS) -I$(EXTSRC)
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

$(OUT)/%.o: %.cpp $(CPPINC)
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

clean:
	rm -rf $(OUT) $(TAGS)
