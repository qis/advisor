# usage: make run|install|format|clean
#
#  run        - build and execute program
#  install    - build and install program
#  format     - use clang-format on all sources
#  clean      - remove curent build directory
#
MAKEFLAGS += --no-print-directory

TARGET	:= $(shell cmake -P res/scripts/target.cmake 2>&1)
SOURCE	:= $(shell cmake -P res/scripts/source.cmake 2>&1)

all: build/release/CMakeCache.txt
	@cmake --build build/release --target $(TARGET)

run: build/release/CMakeCache.txt
	@cmake --build build/release --target $(TARGET)
	@build/release/$(TARGET)

install: build/release/CMakeCache.txt
	@cmake --build build/release --target install

format:
	@C:/LLVM/bin/clang-format.exe -i $(SOURCE)

clean:
	@cmake -E remove_directory .vs build

build/release/CMakeCache.txt: CMakeLists.txt
	@cmake -GNinja -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_INSTALL_PREFIX="$(CURDIR)" \
	  -B build/release

.PHONY: all run install format clean
