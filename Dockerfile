FROM ubuntu as compiler

RUN apt update

# Install tzdata without being interactive
RUN ln -fs /usr/share/zoneinfo/America/New_York /etc/localtime
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends tzdata

# Tools to compile:
RUN apt install -y build-essential cmake clang-format shellcheck curl

COPY cmake /compile/cmake
COPY src /compile/src
COPY test /compile/test
COPY vendor /compile/vendor
COPY CMakeLists.txt /compile/CMakeLists.txt
COPY Makefile /compile/Makefile
WORKDIR /compile

RUN make configure
RUN make compile

FROM ubuntu
COPY --from=compiler /compile/build/dist/bin/jsonschema /usr/local/bin/jsonschema
WORKDIR /schema
ENTRYPOINT ["/usr/local/bin/jsonschema"]
