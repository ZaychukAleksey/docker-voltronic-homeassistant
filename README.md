This is a fork from [catalinbordan](https://github.com/catalinbordan/docker-voltronic-homeassistant) version (which, in turn, derives from [ned-kelly](https://github.com/ned-kelly/docker-voltronic-homeassistant)
project). Please, check the origins for details.

The following changes were made:
- Supports PI18 and PI30 protocols (each protocol exposes its own set of sensors in Home Assistant).
- Protocol is detected automatically.
- Reduced the size of the docker image.
- Made some parameters as changeable right from Home Assistant.
- Reduced the number of MQTT messages (only changed parameters are sent).
- Polling interval is configurable.
- Code simplified and rewritten on C++20 using [Google Style](https://google.github.io/styleguide/cppguide.html).
- Enhancements in logging and error-handling.

<details><summary><b>This is WORK-IN-PROGRESS. Coming soon:</b></summary>

- PI17 protocol support.
- Interactive integration with HomeAssistant (change inverter parameters right from HomeAssistant).
</details>

---

# A Docker-based Home Assistant interface for MPP/Voltronic Solar Inverters 

**Docker Hub:** [`zaychukaleksey/ha-voltronic-mqtt:latest`](https://hub.docker.com/repository/docker/zaychukaleksey/ha-voltronic-mqtt/general)

---

This project [was derived](https://github.com/manio/skymax-demo) from the 'skymax' [C based monitoring application](https://skyboo.net/2017/03/monitoring-voltronic-power-axpert-mex-inverter-under-linux/) designed to
take the monitoring data from Voltronic, Axpert, Mppsolar PIP, Voltacon, Effekta, and other branded
OEM Inverters and send it to a [Home Assistant](https://www.home-assistant.io/) MQTT server for ingestion...

The program can also receive commands from Home Assistant (via MQTT) to change the state of the
inverter remotely.

By remotely setting values via MQTT you can implement many more complex forms of automation
_(triggered from Home Assistant)_ such as:

 - Changing the power mode to '_solar only_' during the day, but then change back to '_grid mode charging_' for your AGM or VLRA batteries in the evenings, but if it's raining (based on data from your weather station), set the charge mode to `PCP02` _(Charge based on 'Solar and Utility')_ so that the following day there's plenty of juice in your batteries...

 - Programmatically set the charge & float voltages based on additional sensors _(such as a Zigbee [Temperature Sensor](https://www.zigbee2mqtt.io/devices/WSDCGQ11LM.html), or a [DHT-22 + ESP8266](https://github.com/bastianraschke/dht-sensor-esp8266-homeassistant))_ - This way if your battery box is too hot/cold you can dynamically adjust the voltage so that the batteries are not damaged...

 - Dynamically adjust the inverter's "solar power balance" and other configuration options to ensure that you get the most "bang for your buck" out of your setup... 

--------------------------------------------------

The program is designed to be run in a Docker Container, and can be deployed on a lightweight SBC
next to your Inverter (i.e. an Orange Pi Zero running Arabian), and read data via the RS232 or USB
ports on the back of the Inverter.

![Example Lovelace Dashboard](https://github.com/ned-kelly/docker-voltronic-homeassistant/raw/master/images/lovelace-dashboard.jpg "Example Lovelace Dashboard")
_Example #1: My "Lovelace" dashboard using data collected from the Inverter & the ability to change
modes/configuration via MQTT._

![Example Lovelace Dashboard](https://github.com/ned-kelly/docker-voltronic-homeassistant/raw/master/images/grafana-example.jpg "Example Grafana Dashboard")
_Example #2: Grafana summary allowing more detailed analysis of data collected, and the ability to
'deep-dive' historical data._


## Prerequisites

- Docker-compose
- [Voltronic/Axpert/MPPSolar](https://www.ebay.com.au/sch/i.html?_from=R40&_trksid=p2334524.m570.l1313.TR11.TRC1.A0.H0.Xaxpert+inverter.TRS0&_nkw=axpert+inverter&_sacat=0&LH_TitleDesc=0&LH_PrefLoc=2&_osacat=0&_odkw=solar+inverter&LH_TitleDesc=0) based inverter that you want to monitor
- Home Assistant [running with an MQTT Server](https://www.home-assistant.io/components/mqtt/)


## Configuration & Standing Up

It's pretty straightforward, just clone down the sources and set the configuration files in the
`config/` directory:

```bash
# Clone down sources on the host you want to monitor.
git clone https://github.com/ZaychukAleksey/voltronic-homeassistant.git /opt/voltronic-homeassistant
cd /opt/voltronic-homeassistant

# Configure inverter connection and MQTT settings.
# Please, read descriptions in inverter.conf.
nano inverter.conf

# Then, plug in your Serial or USB cable to the Inverter & stand up the container.
#     --detach: detached mode, container is run in the background.
docker-compose up --detach
```

_**Note:**_

  - builds on docker hub are currently for `linux/amd64,linux/arm/v6,linux/arm/v7,linux/arm64,linux/386` -- If you have issues standing up the image on your Linux distribution (i.e. An old Pi/ARM device) you may need to manually build the image to support your local device architecture - This can be done by uncommenting the build flag in your docker-compose.yml file.

  - The default `docker-compose.yml` file includes Watchtower, which can be  configured to auto-update this image when we push new changes to github - Please **uncomment if you wish to auto-update to the latest builds of this project**.

## Integrating into Home Assistant.

Providing you have setup [MQTT](https://www.home-assistant.io/components/mqtt/) with Home Assistant,
the device will be automatically registered in your Home Assistant when the container starts for the
first time – you do not need to manually define any sensors.

From here you can setup [Graphs](https://www.home-assistant.io/lovelace/history-graph/) to display sensor data, and optionally change inverter
parameters.

### Using `inverter_poller` binary directly

To compile inverter_poller binary locally, run: `cmake . && make`.

Please, run `./inverter_poller --help` to see supported commands/arguments.

### Bonus: Lovelace Dashboard Files

_**Please refer to the screenshot above for an example of the dashboard.**_

I've included some Lovelace dashboard files in the `homeassistant/` directory, however you will
need to need to adapt to your own Home Assistant configuration and/or name of the inverter if you
have changed it in the `mqtt.json` config file.

Note that in addition to merging the sample Yaml files with your Home Assistant, you will need the
following custom Lovelace cards installed if you wish to use my templates:

 - [vertical-stack-in-card](https://github.com/custom-cards/vertical-stack-in-card)
 - [circle-sensor-card](https://github.com/custom-cards/circle-sensor-card)


 I have put in home assistant folder the examples of what I have for sending commands to inverter, template sensor for warnings, etc. 
For the input buttons you will use the helpers in HA to create them. 
The names and the option of them look in automations examples.
