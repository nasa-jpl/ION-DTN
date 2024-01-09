# ION Design and API Overview

## Basic Philosophy

ION began its development in the early 2000's focusing on flight system running Real-time Operating System (RTOS) with minimum resources under strict control. While these constraints might be somewhat relaxed for modern embedded systems, ION's light weight, modular, and portal traits still remain desirable to both flight and ground systems:

**Hard Memory Allocation Limits**: ION operates within a host-specified memory allocation, managing dynamic allocation internally via a private memory management system. This approach ensures efficient use of the allocated memory resources.

**Modular and Robust Operation**: ION's design allows for individual modules to be started, stopped, rebuilt, or possibly replaced independently. This modular structure is implemented through separate daemons and libraries, enhancing system resilience. In the event of a process crash, data in the process's queues/buffers can be preserved in the non-volatile SDR, preventing data loss.

**Efficient Resource Utilization**: ION is optimized for environments with limited memory, storage, and processing resources. It avoids duplicate data copies during multi-stage processing by utilizing Zero-Copy Objects (ZCO) in shared-memory for fast hand-off between modules. This method, while more complex, ensures rapid data handling. Additionally, BP and CLA services operate as background daemons to minimize competition with critical spacecraft functions during nominal, high-stress operation and off-nominal events.

**Independence from Native IP Socket Support**: ION employs software abstraction to decouple socket-based programming from its core functionalities. This allows ION to interface the Bundle Protocol and CLAs with various underlying communication systems, such as CCSDS space links or radio communications systems or customized processing chains that are not IP-based.

**Portability and Minimal Footprint for Static Linking**: ION prioritizes portability and minimal resource footprint by building its own function libraries. This approach supports static linking through the ION-core package for specific set of modules and reduces dependency on external libraries, thereby mitigating the risk of interference from unexercised or non-required code that cannot be removed from the libraries. This design also avoids potential compatibility issues between the target system’s build environment and those of externally-sourced libraries.

## API Overview

In the [BP Service API document](./BP-Service-API.md) we shown the default installation location of various libraries and daemons. Interactions with these daemons relies on the use of various ION APIs available in the libraries. The following diagram shows the basic ION architecture and different categories of APIs available:

