-include Makefile.config

LIBDRC_SRCS:=$(wildcard src/*.cpp)
LIBDRC_OBJS:=$(LIBDRC_SRCS:.cpp=.o)

ifeq ($(DEMOS),y)
DEMOS_NAMES:=3dtest tsdraw simpleaudio
DEMOS_SRCS:=demos/framework/framework.cpp \
            $(foreach d,$(DEMOS_NAMES),demos/$(d)/main.cpp)
DEMOS_BINS:=$(foreach d,$(DEMOS_NAMES),demos/$(d)/$(d))
endif

ALL_SRCS:=$(LIBDRC_SRCS) $(DEMOS_SRCS)
ALL_OBJS:=$(ALL_SRCS:.cpp=.o)
ALL_DEPENDS:=$(ALL_SRCS:.cpp=.d)

all: libdrc demos

libdrc: libdrc.so libdrc.a

demos: $(DEMOS_BINS)

libdrc.a: $(LIBDRC_OBJS)
	rm -f $@
	ar rcs $@ $^

libdrc.so: $(LIBDRC_OBJS) Makefile.config
	$(CXX) $(LDFLAGS) -shared -o $@ $(LIBDRC_OBJS)

define build_demo
demos/$(1)/$(1): demos/$(1)/main.o demos/framework/framework.o libdrc.a
	$$(CXX) -o $$@ $$^ $$(LDFLAGS) $$(LDFLAGS_DEMOS)
endef
$(foreach d,$(DEMOS_NAMES),$(eval $(call build_demo,$(d))))

%.o %.d: %.cpp Makefile.config
	$(CXX) -MD -c $(CXXFLAGS) $< -o $(<:.cpp=.o)

clean:
	rm -f $(ALL_OBJS) $(ALL_DEPENDS)

distclean: clean
	rm -f libdrc.pc Makefile.config libdrc.a libdrc.so $(DEMOS_BINS)

install: all
	install -d $(PREFIX)/lib $(PREFIX)/lib/pkgconfig
	install -d $(PREFIX)/include/drc $(PREFIX)/include/drc/c
	install libdrc.a libdrc.so $(PREFIX)/lib
	install libdrc.pc $(PREFIX)/lib/pkgconfig
	install include/drc/*.h $(PREFIX)/include/drc
	install include/drc/c/*.h $(PREFIX)/include/drc/c

uninstall:
	@echo TODO

doc:
	make -C doc html

Makefile.config:
	$(error You must run ./configure first)

-include $(ALL_DEPENDS)

.PHONY: all libdrc demos clean distclean install uninstall doc
