.DELETE_ON_ERROR:
.PHONY: all clean
.DEFAULT_GOAL:=all

OUT:=.out
TAGS:=.tags
BINOUT:=$(OUT)
BIN:=$(BINOUT)/catena

WFLAGS:=-Wall -W -Werror
OFLAGS:=-O2
CXXFLAGS:=-pipe -std=c++14
CXXFLAGS:=$(CXXFLAGS) $(WFLAGS) $(OFLAGS)

all: $(TAGS) $(BIN) $(TESTBINS)

$(BINOUT)/catena: src/catena/catena.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Generated targets which live outside of $(OUT)
$(TAGS):
	ctags -R .

clean:
	rm -rf $(OUT)
	rm -rf $(TAGS)
