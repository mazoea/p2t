.PHONY: clean

REL_ADDR = ../../
include $(REL_ADDR)/Makefile.rules

CXX_SRC = \
	format.cc

HEADERS = \
	format.h

CXX_OBJS = \
    format.o

TARGET = libformat.a

all: $(TARGET)

$(TARGET): deps_cxx $(CXX_OBJS)
	$(DEL_FILE) $@
	$(AR) $@ $(CXX_OBJS)
	$(RANLIB) $@

clean:
	$(DEL_FILE) *.o
	$(DEL_FILE) $(TARGET) deps_cxx

deps_cxx: $(HEADERS)
	$(CXX) $(CXXFLAGS) $(MANDATORY_INCPATH) -M -MF deps_cxx $(CXX_SRC)

include deps_cxx
