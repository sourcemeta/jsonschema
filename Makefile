# Programs
CMAKE = cmake
CTEST = ctest
CPACK = cpack
NPM = npm
NODE = node
DOCKER = docker

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
		--component sourcemeta_jsonschema
	$(CPACK) --config build/CPackConfig.cmake -B build/out -C $(PRESET)

test: .always
	$(CTEST) --test-dir ./build --build-config $(PRESET) \
		--output-on-failure --progress --parallel

clean: .always
	$(CMAKE) -E rm -R -f build

node_modules: package.json package-lock.json
	$(NPM) ci

npm-pack: node_modules .always
	$(NPM) --version
	$(CMAKE) -P cmake/fetch-github-releases.cmake
	$(NODE) npm/cli.js
	$(NODE) node_modules/eslint/bin/eslint.js npm/*.js npm/*.mjs
	$(NODE) --test npm/*.test.js npm/*.test.mjs
	$(CMAKE) -E make_directory ./build/npm/dist
	$(NPM) pack --pack-destination ./build/npm/dist

npm-publish: npm-pack
	$(NPM) publish

alpine: .always
	$(DOCKER) build --progress plain --file Dockerfile.test.alpine .

# For NMake, which doesn't support .PHONY
.always:
