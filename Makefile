.DELETE_ON_ERROR:
.PHONY: all bin valgrind check test docker docker-debbin debsrc debbin clean
.DEFAULT_GOAL:=all

VERSION:=$(shell dpkg-parsechangelog -SVersion)

SRC:=src
OUT:=.out
TAGS:=.tags
BINOUT:=$(OUT)
BIN:=$(BINOUT)/catena
TESTBIN:=$(BINOUT)/catenatest
VALGRIND:=valgrind --tool=memcheck --leak-check=full

CAPNPLIBS:=$(shell pkg-config --libs capnp)
CAPNPCFLAGS:=$(shell pkg-config --cflags capnp)
HTTPDLIBS:=$(shell pkg-config --libs libmicrohttpd)
HTTPDCFLAGS:=$(shell pkg-config --cflags libmicrohttpd)
SSLLIBS:=$(shell pkg-config --libs openssl)
SSLCFLAGS:=$(shell pkg-config --cflags openssl)
READLINELIBS:=-lreadline

CPPSRCDIRS:=$(wildcard $(SRC)/*)
CPPSRC:=$(shell find $(CPPSRCDIRS) -type f -iname \*.cpp -print)
CPPINC:=$(shell find $(CPPSRCDIRS) -type f -iname \*.h -print)
VERSIONH:=$(OUT)/version.h # generated by Makefile

PROTOS:=$(shell find $(CPPSRCDIRS) -type f -iname \*.capnp -print)
# Generated Cap'n Proto definitions
PROTOINC:=$(addprefix $(OUT)/, $(PROTOS:%.capnp=%.h))

CATENASRC:=$(foreach dir, $(SRC)/catena $(SRC)/libcatena, $(filter $(dir)/%, $(CPPSRC)))
CATENAOBJ:=$(addprefix $(OUT)/, $(CATENASRC:%.cpp=%.o))
CATENAINC:=$(foreach dir, $(SRC)/catena $(SRC)/libcatena, $(filter $(dir)/%, $(CPPINC))) $(PROTOINC)
CATENATESTSRC:=$(foreach dir, $(SRC)/test $(SRC)/libcatena, $(filter $(dir)/%, $(CPPSRC)))
CATENATESTOBJ:=$(addprefix $(OUT)/, $(CATENATESTSRC:%.cpp=%.o))
CATENATESTINC:=$(foreach dir, $(SRC)/test $(SRC)/libcatena, $(filter $(dir)/%, $(CPPINC))) $(PROTOINC)
# libcatena is not its own binary, just a namespace; no src/obj rules necessary
LIBCATENAINC:=$(foreach dir, $(SRC)/libcatena, $(filter $(dir)/%, $(CPPINC))) $(PROTOINC)

LEDGER:=genesisblock
TESTDATA:=$(wildcard test/*) $(LEDGER)
DOCKERFILE:=Dockerfile

WFLAGS:=-Wall -W -Werror -Werror=vla
# clang doesn't like this
# WFLAGS+=-Wl,-z,defs
OFLAGS:=-g -O2
CPPFLAGS:=-I$(SRC) -I$(OUT)/$(SRC)
CXXFLAGS:=-pipe -std=c++17 -pthread
EXTCPPFLAGS:=$(SSLCFLAGS) $(HTTPDCFLAGS) $(CAPNPCFLAGS)
CXXFLAGS:=$(CXXFLAGS) $(WFLAGS) $(OFLAGS) $(CPPFLAGS) $(EXTCPPFLAGS)

# FIXME detect this, or let it be specified
LDLIBSGTEST:=/usr/lib/x86_64-linux-gnu/libgtest.a

all: $(TAGS) $(BIN) $(TESTBIN)

$(BINOUT)/catena: $(CATENAOBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SSLLIBS) $(HTTPDLIBS) $(READLINELIBS) $(CAPNPLIBS)

$(BINOUT)/catenatest: $(CATENATESTOBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBSGTEST) $(SSLLIBS) $(CAPNPLIBS)

$(OUT)/$(SRC)/catena/%.o: $(SRC)/catena/%.cpp $(CATENAINC) $(VERSIONH)
	@mkdir -p $(@D)
	$(CXX) -include $(VERSIONH) $(CXXFLAGS) -c $< -o $@

$(OUT)/$(SRC)/test/%.o: $(SRC)/test/%.cpp $(CATENATESTINC)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)/$(SRC)/libcatena/%.o: $(SRC)/libcatena/%.cpp $(LIBCATENAINC)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)/$(SRC)/proto/%.h: $(SRC)/proto/%.capnp
	@mkdir -p $(@D)
	capnp compile -oc++:$(OUT) $<

# Generated targets which live outside of $(OUT)
$(TAGS): $(CPPSRC) $(CPPINC)
	ctags -f $@ $^ || true

bin: $(BIN)

check: test

$(VERSIONH): debian/changelog
	@mkdir -p $(@D)
	echo "#ifndef CATENA_CATENA_VERSION\n#define CATENA_CATENA_VERSION\nnamespace CatenaAgent{constexpr const char VERSION[] = \"$(VERSION)\";}\n#endif" > $@

test: $(TAGS) $(TESTBIN) $(TESTDATA)
	$(BINOUT)/catenatest

valgrind: $(TAGS) $(TESTBIN) $(TESTDATA)
	$(VALGRIND) $(BINOUT)/catenatest

docker: $(DOCKERFILE)
	docker build -f $< .

debsrc:
	dpkg-source --build .

debbin: debsrc
	@mkdir -p $(OUT)/deb
	sudo pbuilder build --buildresult $(OUT)/deb ../*dsc

# Used internally by Dockerfile
docker-debbin:
	@mkdir -p $(OUT)/deb
	debuild -uc -us

clean:
	rm -rf $(OUT) $(TAGS)
