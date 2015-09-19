MELTED_SRC_DIR = $(realpath ../melted)
MELTED_INSTALL_DIR = $(shell pwd)/build
MELTED_ARTIFACT = $(shell pwd)/build/bin/melted

MELTED_CONFIG_OPTS = --enable-gpl --prefix=$(MELTED_INSTALL_DIR)

$(MELTED_ARTIFACT):
	cd $(MELTED_SRC_DIR) ; PKG_CONFIG_PATH=$(MLT_INSTALL_DIR)/lib/pkgconfig \
	./configure $(MELTED_CONFIG_OPTS) ; make -j8 ; make install

clean-melted:
	cd $(MELTED_SRC_DIR) ; make clean

melted:
	cd $(MELTED_SRC_DIR) ; make -j8 && make install



