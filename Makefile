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
CPPSRC:=$(shell find $(CPPSRCDIRS) -type f -iname \*.cpp -print)
CPPINC:=$(shell find $(CPPSRCDIRS) -type f -iname \*.h -print)

CATENASRC:=$(foreach dir, src/catena src/libcatena, $(filter $(dir)/%, $(CPPSRC)))
CATENAOBJ:=$(addprefix $(OUT)/,$(CATENASRC:%.cpp=%.o))
CATENATESTSRC:=$(foreach dir, src/test src/libcatena, $(filter $(dir)/%, $(CPPSRC)))
CATENATESTOBJ:=$(addprefix $(OUT)/,$(CATENATESTSRC:%.cpp=%.o))

WFLAGS:=-Wall -W -Werror -Wl,-z,defs
OFLAGS:=-O2
CXXFLAGS:=-pipe -std=c++14 -pthread
CXXFLAGS:=$(CXXFLAGS) $(WFLAGS) $(OFLAGS)

# FIXME detect this, or let it be specified
LDLIBSGTEST:=/usr/lib/x86_64-linux-gnu/libgtest.a

all: $(TAGS) $(BIN) $(TESTBIN)

$(BINOUT)/catena: $(CATENAOBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BINOUT)/catenatest: $(CATENATESTOBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBSGTEST)

$(OUT)/%.o: %.cpp $(CPPINC)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

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
