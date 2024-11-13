FROM debian:bookworm as builder
RUN apt-get --yes update && apt-get install --yes --no-install-recommends \
  build-essential cmake && apt-get clean && rm -rf /var/lib/apt/lists/*

COPY cmake /source/cmake
COPY src /source/src
COPY vendor /source/vendor
COPY CMakeLists.txt /source/CMakeLists.txt

RUN cmake -S /source -B ./build -DCMAKE_BUILD_TYPE:STRING=Release -DBUILD_SHARED_LIBS:BOOL=OFF
RUN cmake --build /build --config Release --parallel 4
RUN cmake --install /build --prefix /usr/local --config Release --verbose --component sourcemeta_jsonschema

FROM debian:bookworm-slim
COPY --from=builder /usr/local/bin/jsonschema /usr/local/bin/jsonschema
WORKDIR /workspace
ENTRYPOINT [ "/usr/local/bin/jsonschema" ]
