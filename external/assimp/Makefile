.PHONY: all configure build install clean

ASSIMP_BUILD_DIR = _build/assimp-makefile
ASSIMP_INSTALL_PREFIX = $(CURDIR)/_build/assimp-makefile/_install
all: build

configure:
	mkdir -p $(ASSIMP_BUILD_DIR)
	cmake \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo \
		-DASSIMP_BUILD_TESTS=OFF \
		-DCMAKE_INSTALL_PREFIX=$(ASSIMP_INSTALL_PREFIX) \
		-B $(ASSIMP_BUILD_DIR) \
		-S assimp-master

build: configure
	make -C $(ASSIMP_BUILD_DIR) -j$(nproc)

install: build
	make -C $(ASSIMP_BUILD_DIR) install

clean:
	rm -rf _build
	@echo "Cleaned _build"

