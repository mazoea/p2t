REL_ADDR = ../../
include $(REL_ADDR)/Makefile.rules

CXX_SRC = \
	pdf_to_text.cc \
	pdf_to_ppm.cc \
	pdf_to_png.cc

HEADERS = 

CXX_OBJS = 

.PHONY: all clean
all: deps_cxx pdf_to_text pdf_to_ppm pdf_to_png

pdf_to_text: pdf_to_text.o
	$(DEL_FILE) $@
	$(LINK) $(STANDARD_LDFLAGS) $(MANDATORY_INCPATH) -o $@ $@.o $(MANDATORY_LIBS) 

pdf_to_ppm: pdf_to_ppm.o
	$(DEL_FILE) $@
	$(LINK) $(STANDARD_LDFLAGS) $(MANDATORY_INCPATH) -o $@ $@.o $(MANDATORY_LIBS) 

pdf_to_png: pdf_to_png.o
	$(DEL_FILE) $@
	$(LINK) $(STANDARD_LDFLAGS) $(MANDATORY_INCPATH) -o $@ $@.o $(MANDATORY_LIBS) 

clean:
	$(DEL_FILE) *.o
	$(DEL_FILE) $(TARGET) deps_cxx
 
deps_cxx: $(HEADERS)
	$(CXX) $(CXXFLAGS) $(STANDARD_LDFLAGS) $(MANDATORY_INCPATH) -M -MF deps_cxx $(CXX_SRC)

#include deps_cxx
