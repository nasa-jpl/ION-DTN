# ION Configuration Tools

ION provides several tools for generating and managing node configurations. Choose the tool that best fits your environment and needs.

## Official Tools

**[ION Config Tool](https://github.com/nasa-jpl/ion-config-tool)** - The official, full-featured, web GUI configuration tool for ION. It supports all ION configuration options. It also includes the **ionlauncher** tool - a simplified command-line utility to generate ION configurations and launch ION nodes.

**[ION Network Model](https://github.com/nasa-jpl/ion-network-model)** - A graphical frontend to specify a network of computer hosts, DTN nodes, IP address assignments, and link connections in a JSON file. The output can be imported into the ION Config Tool to generate ION configuration files.

## Additional Utilities (planned for 4.2)

**[ionrun](ionrun.md)** - Simple configuration in a terminal-only environment. No Python, no Java dependencies. Suitable for developers on embedded systems to quickly generate configurations for unit testing with a reduced option set in a terminal session. It can ingest configuration output produced by the ION Config Tool.

**ION Dev Kit** (re-introduced) - A framework to set up and launch multi-node ION network scenarios using the CORE emulator for visualization. The ION Dev Kit was previously discontinued after ION 3.6. It is under development and planned to be re-introduced for ION 4.2.
