FROM debian:trixie AS builder
RUN apt-get --yes update && apt-get install --yes --no-install-recommends \
  build-essential cmake libcurl4-openssl-dev && apt-get clean && rm -rf /var/lib/apt/lists/*

COPY cmake /source/cmake
COPY src /source/src
COPY completion /source/completion
COPY vendor /source/vendor
COPY DEPENDENCIES /source/DEPENDENCIES
COPY CMakeLists.txt /source/CMakeLists.txt
COPY VERSION /source/VERSION

RUN cmake -S /source -B ./build \
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -DBUILD_SHARED_LIBS:BOOL=OFF \
  -DJSONSCHEMA_PORTABLE:BOOL=ON \
  -DJSONSCHEMA_USE_SYSTEM_CURL:BOOL=ON
RUN cmake --build /build --config Release --parallel 4
RUN cmake --install /build --prefix /usr/local --config Release --verbose --component sourcemeta_jsonschema

FROM debian:trixie-slim
RUN apt-get --yes update && apt-get install --yes --no-install-recommends \
  libcurl4t64 && apt-get clean && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/bin/jsonschema /usr/local/bin/jsonschema
WORKDIR /workspace
ENTRYPOINT [ "/usr/local/bin/jsonschema" ]
