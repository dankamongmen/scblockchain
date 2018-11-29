.DELETE_ON_ERROR:
.PHONY: all bin check test clean
.DEFAULT_GOAL:=all

SRC:=src
OUT:=.out
TAGS:=.tags
BINOUT:=$(OUT)
BIN:=$(BINOUT)/catena
TESTBIN:=$(BINOUT)/catenatest

CPPSRCDIRS:=$(wildcard $(SRC)/*)
CPPSRC:=$(shell find $(CSRCDIRS) -type f -iname \*.cpp -print)
CPPINC:=$(shell find $(CSRCDIRS) -type f -iname \*.h -print)
CPPOBJ:=$(addprefix $(OUT)/,$(CPPSRC:%.c=%.o))

WFLAGS:=-Wall -W -Werror -Wl,-z,defs
OFLAGS:=-O2
CXXFLAGS:=-pipe -std=c++14 -pthread
CXXFLAGS:=$(CXXFLAGS) $(WFLAGS) $(OFLAGS)

# FIXME detect this, or let it be specified
LDLIBSGTEST:=/usr/lib/x86_64-linux-gnu/libgtest.a

all: $(TAGS) $(BIN) $(TESTBIN)

$(BINOUT)/catena: src/catena/catena.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BINOUT)/catenatest: src/test/catenatest.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDLIBSGTEST)

# Generated targets which live outside of $(OUT)
$(TAGS): $(CPPSRC) $(CPPINC)
	ctags -f $@ $^

bin: $(BIN)

check: test

test: $(TESTBIN)
	$(BINOUT)/catenatest

clean:
	rm -rf $(OUT)
	rm -rf $(TAGS)
