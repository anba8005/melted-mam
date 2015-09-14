DEFAULT_GOAL := melted-mam

include mk/mlt.mk
include mk/melted.mk

CFLAGS = -O2 -Wno-write-strings
MELTED_MAM_INCLUDE = -Ibuild/include/mlt++ -Ibuild/include/mlt -Ibuild/include/mlt/melted++
MELTED_MAM_LIB = -Wl,-rpath=/usr/local/mam/lib -L/usr/local/mam/lib -lmelted++ -lmelted -lmlt++ -lmlt -lmvcp

deps: $(MLT_ARTIFACT) $(MELTED_ARTIFACT)

clean: clean-mlt clean-melted
	rm -rf $(shell pwd)/build

all: melted-mam

melted-mam: deps
	g++ -o build/bin/melted-mam $(CFLAGS) $(MELTED_MAM_INCLUDE) src/melted-mam.cpp $(MELTED_MAM_LIB) 
	