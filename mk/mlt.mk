MLT_SRC_DIR = $(realpath ../mlt)
MLT_INSTALL_DIR = $(shell pwd)/build
MLT_ARTIFACT = $(shell pwd)/build/lib/libmlt.so.6

FFMPEG_DIR = $(realpath ../FFmpeg-build/build)

MLT_CONFIG_OPTS = --enable-gpl --enable-linsys --prefix=$(MLT_INSTALL_DIR) \
--avformat-static=$(FFMPEG_DIR) --without-kde --disable-frei0r \
--avformat-ldextra="-lstdc++ -ldl -lasound -lSDL $(FFMPEG_DIR)/lib/libfaac.a \
$(FFMPEG_DIR)/lib/libx264.a $(FFMPEG_DIR)/lib/librtmp.a"

CFLAGS += -I$(MLT_INSTALL_DIR)/include
LDFLAGS += -L$(MLT_INSTALL_DIR)/lib

$(MLT_ARTIFACT):
	cd $(MLT_SRC_DIR) ; ./configure $(MLT_CONFIG_OPTS) ; make -j8 ; make install

clean-mlt:
	cd $(MLT_SRC_DIR) ; make clean




