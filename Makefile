# top-level makefile 

.PHONY: distclean clean source install install-no all

# includes basic building rules
include Makefile.rules

all: source 

# Common install target. It depends on configuration which specific 
# installation target will be used.
install:  $(INSTALL_TARGET) 

# make application
source: 
	cd $(SRCROOT) && $(MAKE)

# cleanup
clean:
	cd $(SRCROOT) && $(MAKE) clean || true
	$(DEL_FILE) config.log

# dist cleanup
distclean:
	cd $(SRCROOT) && $(MAKE) distclean || true
	$(DEL_FILE) config.status config.log Makefile.flags Makefile.rules|| true
	$(DEL_FILE) autom4te.cache/* || true
	$(DEL_DIR) autom4te.cache || true
