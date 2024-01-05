# To build this image locally:
# docker build --progress=plain -t voltronic .
#
# To rebuild from scratch:
# docker image rm ...
# docker builder prune --all --force
# docker build --progress=plain -t voltronic .
#
# NOTE: if you're trying to run the image without docker-compose, when you should mount "config"
# directory: docker run -itv <absolute_path>/inverter.conf:/inverter.conf voltronic
#
# To get into the container itsef (e.g. for debugging purposes):
# docker run -itv <absolute_path>/inverter.conf:/inverter.conf --entrypoint sh voltronic
#
# To build multiarch image and push it into the repo:
# docker buildx build --platform=linux/amd64,linux/arm/v6,linux/arm/v7,linux/arm64,linux/386 -t zaychukaleksey/ha-voltronic-mqtt:latest --push .

# Multi-stage build is used to minify the result image: https://docs.docker.com/build/building/multi-stage/.
# https://hub.docker.com/_/alpine/tags
FROM alpine:latest AS build_image

# Install required packages to build the inverter poller binary.
RUN apk update && apk add --no-cache g++ make cmake openssl-dev

# Copy sources.
COPY . /build/

# Build the binary
RUN cd /build && cmake -DCMAKE_BUILD_TYPE=Release . && make -j8


FROM alpine:latest

# Copy inverter_poller binary from the build image into our result image.
COPY --from=build_image /build/src/inverter_poller /

ENTRYPOINT ["/inverter_poller -d"]
