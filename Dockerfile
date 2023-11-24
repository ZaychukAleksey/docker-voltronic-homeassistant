# To build this image locally:
# docker build --progress=plain -t voltronic .
#
# To rebuild from scratch:
# docker image rm ...
# docker builder prune --all --force
# docker build --progress=plain -t voltronic .
#
# NOTE: if you're trying to run the image without docker-compose, when you should mount "config"
# directory: docker run -it -v <absolute_path>/config:/opt/config voltronic
#
# To build multiarch image and push it into the repo:
# docker buildx build --platform=linux/amd64,linux/arm/v6,linux/arm/v7,linux/arm64,linux/386 -t zaychukaleksey/ha-voltronic-mqtt:latest --push .

# Multisource-build is used to minify the result image. https://docs.docker.com/build/building/multi-stage/
# https://hub.docker.com/_/alpine/tags
FROM alpine:latest AS build_image

# Install required packages to build the inverter poller binary.
RUN apk update && apk add --no-cache g++ make cmake

# Copy sources.
COPY sources/inverter-cli /opt/

# Build the binary
RUN cd /opt && cmake . && make -j2


FROM alpine:latest

# Install packages, required for mqtt-related scripts.
RUN apk update && apk add --no-cache bash coreutils jq mosquitto-clients libstdc++

# Copy inverter_poller binary from the build image into our result image.
COPY --from=build_image /opt/inverter_poller /opt

# Copy mqtt-related scripts.
COPY sources/inverter-mqtt /opt/inverter-mqtt

# https://docs.docker.com/engine/reference/builder/#healthcheck
HEALTHCHECK \
    --interval=30s \
    --timeout=10s \
    --start-period=1m \
    --retries=3 \
  CMD /opt/healthcheck

WORKDIR /opt
ENTRYPOINT ["/bin/bash", "/opt/inverter-mqtt/entrypoint.sh"]
