FROM ubuntu as compiler

RUN apt update

# Install tzdata without being interactive
RUN ln -fs /usr/share/zoneinfo/America/New_York /etc/localtime
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends tzdata

# Tools to compile:
RUN apt install -y build-essential cmake

COPY cmake /compile/cmake
COPY src /compile/src
COPY vendor /compile/vendor
COPY CMakeLists.txt /compile/CMakeLists.txt
WORKDIR /compile

RUN cmake -S . -B ./build -DCMAKE_BUILD_TYPE:STRING=Release -DBUILD_SHARED_LIBS:BOOL=OFF
RUN cmake --build ./build --config Release --parallel 4
RUN cmake --install ./build --prefix /usr/local --config Release --verbose --component intelligence_jsonschema

FROM ubuntu
COPY --from=compiler /usr/local/bin/jsonschema /usr/local/bin/jsonschema
WORKDIR /schema
ENTRYPOINT ["/usr/local/bin/jsonschema"]
