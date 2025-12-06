# Interplanetary Overlay Network (ION)

## ION Description

**Interplanetary Overlay Network (ION)** is an implementation of the DTN architecture, as described in Internet RFC 4838 (version 6) and RFC 9171 (version 7), that is intended to be usable in both embedded environments including spacecraft flight computers as well as ground systems. It includes modular software packages implementing Bundle Protocol version 7 (BPv7), Licklider Transmission Protocol (LTP), Bundle Streaming Service (BSS), DTN-based CCSDS File Delivery Protocol (CFDP), Asynchronous Message Service (AMS), and several other DTN services and prototypes. ION is currently the baseline implementation for science instruments on the International Space Station (ISS) and the gateway node (ION Gateway) that provides relay services for command/telemetry and science data download.

Here you will find videos of the Interplanetary Overlay Network [courses and presentation materials](https://www.nasa.gov/directorates/heo/scan/engineering/technology/disruption_tolerant_networking_software_options_ion).

DTN Development/Deployment Kit is an ISO image of an Ubuntu virtual machine, pre-configured with ION and a GUI virtualization environment. It contains a number of pre-built scenarios (network topologies) to demonstrate various features of ION software. (**currently the DevKit is undergoing upgrade to BPv7, release date is TBD.**)

## Application Domains of DTN and ION

- **Robotics**: This technology enables us to command and control robotic explorers on distant planets and support timely decision-making despite the limitations imposed by vast distances.
- **Satellite Communications**: Our protocol enhances satellite constellations by providing a solution tailored to the unique challenges posed by space communications.
- **Spacecraft Fleets**: Our system can coordinate entire fleets of spacecraft engaged in deep space missions, ensuring synchronized actions and efficient resource use.
- **Interplanetary Internet**: We're laying the foundation for an internet that extends beyond Earth to span across multiple planets, paving the way for universal communication.
- **Space Station Data Handling**: Our technology efficiently manages data transfer between Earth and Space Station's various science payload and instruments, ensuring timely and reliable communications.

## Performance Data

- **BP/LTP Throughput Study**: A comprehensive study of BP/LTP performance on various CPU/OS architecture will be provided in the [ION Deployment Guide](ION-Deployment-Guide.md).
- **ION TCPCL Throughput Assessment**: An in-depth study conducted at Ohio University on the performance optimization of ION over the TCP convergence layer is available [here](https://etd.ohiolink.edu/acprod/odb_etd/etd/r/1501/10?clear=10&p10_accession_num=ohiou1619115602389023) and as a [pdf](https://etd.ohiolink.edu/acprod/odb_etd/ws/send_file/send?accession=ohiou1619115602389023&disposition=inline) file.

## Installation & Configuration

1. Clone the repository:

   ```bash
   git clone https://github.com/nasa-jpl/ION-DTN.git
   ```

2. Follow the steps in the [Quick Start Guide](./ION-Quick-Start-Guide.md) to build, install, and run a simple two node example.
3. A simple tutorial of ION's configuration files can be found [here](./Basic-Configuration-File-Tutorial.md).
4. A set of configuration file templates for various DTN features can be found [here](./ION-Config-File-Templates.md).

## Operations

- **[ION Utilities](./ION-Utilities.md)**: Overview of utility programs for launching, stopping, and monitoring ION.
- **[ION Shutdown Guide](./ION-Shutdown-Guide.md)**: Comprehensive guide to the various methods for stopping ION nodes and cleaning up system resources.
- **[ION Monitoring Guide](./ION-Monitoring-Guide.md)**: Guide to monitoring ION node status and performance.
- **[SOP for ION](./SOP-for-ION.md)**: Standard Operating Procedures for ION deployment and operations.

## API Documentation

ION provides comprehensive APIs for application development and system administration:

- **[ION Design and API Overview](./ION-Design-and-API-Overview.md)**: Overview of ION's modular architecture and the complete set of APIs available for interacting with DTN services.
- **[Public Administration API Guide](./Public-Admin-API-Guide.md)**: Detailed guide for programmatic configuration and management of ION, LTP, and BP subsystems using the ion_admin.h, ltp_admin.h, and bp_admin.h APIs introduced in ION 4.1.4-b.1. This enables applications to configure ION nodes directly from C code without requiring command-line tools.
- **[BP Service API](./BP-Service-API.md)**: Documentation for Bundle Protocol service APIs that enable applications to send and receive bundles.
- **[IPN Naming Scheme Transition](./IPN-Naming-Scheme-Transition.md)**: Guide for migrating code from the old 2-part IPN node numbers to the new 3-part Fully Qualified Node Numbers (FQNN) introduced in ION 4.1.4-a.1.

## License

ION is licensed under the MIT License. Please see the [LICENSE](./License.md) file for details.

## Important Papers on ION and DTN

For a list of key DTN and ION-related publications, please refer to the [List-of-Papers](./List-of-Papers.md) page.
