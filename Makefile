DEFAULT_GOAL := melted-mam

include mk/mlt.mk
include mk/melted.mk

CFLAGS = -O2 -Wno-write-strings -std=c++11
MELTED_MAM_INCLUDE = -Ibuild/include/mlt++ -Ibuild/include/mlt -Ibuild/include/mlt/melted++
MELTED_MAM_LIB = -Wl,-rpath=/usr/local/mam/lib -L/usr/local/mam/lib -lmelted++ -lmelted -lmlt++ -lmlt -lmvcp
MELTED_MAM_OBJ = tmp/melted-mam.o tmp/Preview.o tmp/MeltedMAM.o

deps: $(MLT_ARTIFACT) $(MELTED_ARTIFACT)

distclean: clean-mlt clean-melted clean
	rm -rf $(shell pwd)/build

clean:
	rm tmp/* build/bin/melted-mam


all: deps build/bin/melted-mam

tmp/%.o: src/%.cpp
	g++ -c $(CFLAGS) $(MELTED_MAM_INCLUDE) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o $@ $<

build/bin/melted-mam: $(MELTED_MAM_OBJ)
	g++ -o $@ $^ $(CFLAGS) $(MELTED_MAM_LIB) 
	