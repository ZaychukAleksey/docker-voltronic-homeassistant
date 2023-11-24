FROM debian:bullseye-slim

# To build this image locally:
# docker build --progress=plain -t voltronic .
#
# To rebuild from scratch:
# docker image rm ...
# docker builder prune --all --force
# docker build --progress=plain -t voltronic .
#
# To build multiarch image and push it into the repo:
# docker buildx build --platform=linux/amd64,linux/arm/v6,linux/arm/v7,linux/arm64,linux/386 -t zaychukaleksey/ha-voltronic-mqtt:latest --push .

# Copy files.
COPY sources/ /opt/

RUN \
    # Install required packages.
    apt update && apt -y install --no-install-recommends \
        # Curl is used to work with InfluxDB
        curl \
        # These are required to build inverter_poller
        g++ make cmake libspdlog-dev \
        # These are required to work with MQTT
        jq \
        mosquitto-clients \
    # Build inverter_poller binary. It will be placed to /opt.
    && cd /opt/inverter-cli && mkdir bin && cmake . && make -j2 && mv inverter_poller /opt/ \
    # Cleanup trash to reduce the size of the container.
    && apt -y clean \
    && apt -y purge g++ make cmake \
    && apt -y autoremove \
    && rm -rf /opt/inverter-cli


# https://docs.docker.com/engine/reference/builder/#healthcheck
HEALTHCHECK \
    --interval=30s \
    --timeout=10s \
    --start-period=1m \
    --retries=3 \
  CMD /opt/healthcheck

WORKDIR /opt
ENTRYPOINT ["/bin/bash", "/opt/inverter-mqtt/entrypoint.sh"]
