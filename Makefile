# Programs
CMAKE = cmake
CTEST = ctest
CPACK = cpack

# Options
PRESET = Debug
SHARED = OFF
PREFIX = ./build/dist

all: configure compile test

configure: .always
	$(CMAKE) -S . -B ./build \
		-DCMAKE_BUILD_TYPE:STRING=$(PRESET) \
		-DCMAKE_COMPILE_WARNING_AS_ERROR:BOOL=ON \
		-DJSONSCHEMA_TESTS:BOOL=ON \
		-DJSONSCHEMA_TESTS_CI:BOOL=OFF \
		-DJSONSCHEMA_CONTINUOUS:BOOL=ON \
		-DJSONSCHEMA_DEVELOPMENT:BOOL=ON \
		-DBUILD_SHARED_LIBS:BOOL=$(SHARED)

compile: .always
	$(CMAKE) --build ./build --config $(PRESET) --target clang_format
	$(CMAKE) --build ./build --config $(PRESET) --target shellcheck
	$(CMAKE) --build ./build --config $(PRESET) --parallel 4
	$(CMAKE) --install ./build --prefix $(PREFIX) --config $(PRESET) --verbose \
		--component intelligence_jsonschema
	$(CPACK) --config build/CPackConfig.cmake -B build/out -C $(PRESET)

lint: .always
	$(CMAKE) --build ./build --config $(PRESET) --target clang_tidy

test: .always
	$(CTEST) --test-dir ./build --build-config $(PRESET) \
		--output-on-failure --progress --parallel

clean: .always
	$(CMAKE) -E rm -R -f build

# For NMake, which doesn't support .PHONY
.always:
