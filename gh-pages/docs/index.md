# Interplanetary Overlay Network (ION)

## 🛰️ ION Description

**Interplanetary Overlay Network (ION)** is an implementation of DTN architecture, as described in Internet RFC 4838 (version 6) and RFC 9171 (version 7), that is intended to be usable in both embedded environments including spacecraft flight computers as well as ground systems. It includes modular software packages implementing Bundle Protocol version 6 (BPv6) and version 7 (BPv7), Licklider Transmission Protocol (LTP), Bundle Streaming Service (BSS), DTN-based CCSDS File Delivery Protocol (CFDP), Asynchronous Message Service (AMS), and several other DTN services and prototypes. ION is currently the baseline implementation for science instruments on the International Space Station (ISS) and the gateway node (ION Gateway) that provides relay services for command/telemetry and science data download.

Here you will find videos of the Interplanetary Overlay Network [courses and presentation materials](https://www.nasa.gov/directorates/heo/scan/engineering/technology/disruption_tolerant_networking_software_options_ion).

DTN Development/Deployment Kit is an ISO image of an Ubuntu virtual machine, pre-configured with ION and a GUI virtualization environment. It contains a number of pre-built scenarios (network topologies) to demonstrate various features of ION software. (** currently the DevKit is undergoing upgrade to BPv7, release date is TBD.**)

## 📡 Use Cases for ION

- **Robotics**: This technology enables us to command and control robotic explorers on distant planets and support timely decision-making despite the limitations imposed by vast distances.
- **Satellite Communications**: Our protocol enhances satellite constellations by providing a solution tailored to the unique challenges posed by space communications.
- **Spacecraft Fleets**: Our system can coordinate entire fleets of spacecraft engaged in deep space missions, ensuring synchronized actions and efficient resource use.
- **Interplanetary Internet**: We're laying the foundation for an internet that extends beyond Earth to span across multiple planets, paving the way for universal communication.
- **Space Station Data Handling**: Our technology efficiently manages data transfer between Earth and Space Station's various science payload and instruments, ensuring timely and reliable communications.

## 📊 Benchmark Tests

- **BP/LTP Throughput Study**: a comprehensive study of BP/LTP performance on various CPU/OS architecture will be provided in the [ION Deployment Guide](https://github.com/nasa-jpl/ION-DTN/blob/current/doc/ION%20Deployment%20Guide.pdf) after ION 4.1.3 is released. (Current estimated time is Fall of 2023)

## 🛠️ Installation and Configuration

1. Clone the repository:

   ```bash
   git clone https://github.com/nasa-jpl/ION-DTN.git
   ```
2. Follow the steps in the [Quick Start Guide](https://github.com/nasa-jpl/ION-DTN/wiki/ION-Quick-Start-Guide)
3. A simple tutorial of ION's configuration files can be found [here](https://github.com/nasa-jpl/ION-DTN/wiki/Basic-Configuration-File-Tutorial-%26-Examples)
4. A set of configuration file templates for various DTN features can be found [here](https://github.com/nasa-jpl/ION-DTN/wiki/ION-Config-File-Templates).

## 📜 License

ION is licensed under the MIT License. Please see the [LICENSE](LICENSE) file for details.

## 🐛 Known Issues

For a complete list of known issues and solutions, please refer to the [Known-Issues](https://github.com/nasa-jpl/ION-DTN/wiki/Known-Issues) Page.

## 🚀 Current Uses of ION & ION-Integrated Systems

- **NASA Deep Space Network**: DTN services is currently provided by the DSN.
- **NASA International Space Station**: Science Payload onboard ISS as well as command/telemetry/science data relay Gateway
- **KARI: Korean PathFinder Lunar Orbiter (KPLO)**: Operates a DTN-payload (DTNPL)
- **Morehead State University 21-m Antenna**: operates a ground DTN node using ION
- **ION Core**: a streamlined packaging of the core DTN features of ION without the experimental features. Customizable for mission infusion. https://github.com/nasa-jpl/ion-core
- **F Prime Open Source Flight Software**: ION BPv7 has been integrated with F Prime. A prototype is now available to the public at https://github.com/fprime-community/fprime-dtn.

## 📚 Important Papers on ION and DTN

For a list of key DTN and ION papers, refer to the [List-of-Papers](https://github.com/nasa-jpl/ION-DTN/wiki/List-of-Papers) page.
