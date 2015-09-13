DEFAULT_GOAL := deps

include mk/mlt.mk
include mk/melted.mk

deps: $(MLT_ARTIFACT) $(MELTED_ARTIFACT)

clean: clean-mlt clean-melted
	rm -rf $(shell pwd)/build
