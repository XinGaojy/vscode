FROM ubuntu:20.04 as builder
RUN apt-get update && apt-get install -y \
  build-essential cmake git ninja-build \
  libprotobuf-dev protobuf-compiler \
  libgrpc++-dev protobuf-compiler-grpc \
  libssl-dev zlib1g-dev liblz4-dev
COPY . /src
WORKDIR /src
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build

FROM ubuntu:20.04
RUN apt-get update && apt-get install -y \
  libssl1.1 liblz4-1 libprotobuf-lite17 libgrpc++1 \
  && rm -rf /var/lib/apt/lists/*
COPY --from=builder /src/build/dfs-server /usr/local/bin/
COPY --from=builder /src/build/dfs-client /usr/local/bin/
ENTRYPOINT ["dfs-server"]
EXPOSE 3000
