target = advisor

all: build/debug/rules.ninja
	@cmake --build build/debug

run: build/debug/rules.ninja
	@cmake --build build/debug --target $(target)
	@build\debug\$(target).exe

install: build/release/rules.ninja
	@cmake --build build/release --target install

format:
	@cmake -P res/format.cmake

clean:
	@cmake -E remove_directory build

build/debug/rules.ninja: CMakeLists.txt
	@cmake -GNinja -DCMAKE_BUILD_TYPE=Debug \
	  -DCMAKE_INSTALL_PREFIX="$(MAKEDIR)" \
	  -B build/debug

build/release/rules.ninja: CMakeLists.txt
	@cmake -GNinja -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_INSTALL_PREFIX="$(MAKEDIR)" \
	  -B build/release
