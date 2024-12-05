# Multicasting over the Interplanetary Internet :rocket:
This project has been developed by Dr Lara Suzuki, a visiting Researcher at NASA JPL.

***NOTE to reader: After ION 4.1, the `imc` multicasting implementation has changed its configuration and no longer requires the `imcadmin` program for configuration. Please see ION 4.1.2 man pages for more information. This tutorial is based on earlier version of ION's imc multicasting. The proper adaptation to latest ION version is left as an exercise for the reader.***

# Multicasting using ION

Multicasting over the Interplanetary Internet uses version 7 of the Bundle Protocol. In a multicasting scenario, we send messages to a multicasting end point and the messages are propagated across the nodes of the network, removing the need to send data to individual nodes one at a time.

Use multicasting when the messages you are sending should be delivered to all the known nodes of the network.

## Executing Multicasting

This tutorial presents the configurations of one host. To create additional hosts, you just need to copy the same configurations and alter the configuraation documents with the appropriate data as explained in our [first tutorial](../dtn-gcp-main/ION-One-Node-on-Cloud-Linux-VM.md).

To execute multicasting:

1. Execute the "execute" file to get ion started
```
$ ./execute
```

2. When performing multicasting, the eid naming scheme is  **imc 'imcfw' 'imcadminep'**

3. Once ION is started you can run the commands to open bpsink to listen to packages sent over DTN. Simply run on your terminal the command below. This command will leave bpsink running on the background. In our configuration file **host.bprc.2** we define interplanetary multicasting EID as 19.0. Messages sent to this EID will be delivered to all the hosts running the same configuration.
```
$ bpsink imc:19.0 &
```
Messages sent on bpsource imc:19.0 will be delivered to all end points registered in the interplanetary multicasting eid. Note that you can also use **bprecvfile** and **bpsendfile** to send images and videos over multicasting.

```
$ bpsendfile ipn:1.1 imc:19.0 image.jpeg
$ bprecvfile imc:19.0 1
```

# Why ION? Digital Communication in Interplanetary Scenarios

In this section I will give a little overview of the basic concepts of the Interplanetary Overlay Network from Nasa.

Digital communication between interplanetary spacecraft and space flight control centres on Earth is subject to constraints that differ in some ways from those that characterize terrestrial communications. 

## Basic Concepts of Interplanetary Overlay Network

Delay-Disruption Tolerant Networking (DTN) is NASA’s solution for reliable, automated network communications in space missions. The [DTN2](http://www.dtnrg.org/docs/presentations/IETF60/dtn-impl-ietf-8-6-04-demmer.pdf) reference implementation of the Delay-Tolerant Networking (DTN) [RFC 4838](https://tools.ietf.org/html/rfc4838)
and the current version of the [Bundle Protocol (BP)](https://tools.ietf.org/html/draft-ietf-dtn-bpbis-31) is the foundation for a wide range of current DTN industrial and research applications. 

The Jet Propulsion Laboratory (JPL) has developed an alternative implementation of BP, named “Interplanetary Overlay Network” (ION). ION addresses those constraints and enables delay-tolerant network communications in interplanetary mission operations. 

## Space communication challenges

- Extreme distances and high rates of data loss due to radio signal interference make communicating between Earth and interplanetary spacecraft a challenge.
- Constant orbital movement of the satellites, 
the link handovers and the acquisition and loss of signal as satellites come into and out of visibility of the ground antena, and discontinuous vehicle operations make the challenge even more difficult, even in near-Earth space.

To give you a sense of signal propagation -> send a message and receive a response (RRT) back:
1. A typical round trip time between two points on the internet is 100ms to 300ms
2. Distance to ISS (throught TDRS - Tracking and Data Relay Satellite) approx 71322km -> round trip time is approx 1200 ms on Ku Link
3. Distance to the moon approx 384400 km - rrt 2560 ms -2,5 seconds
4. Minimun distance to mars: approx 54.6 million km - rrt of approx 6 min
5. Average distance to mars: approx 225 million km - rrt of approx 25 mins
6. Farthest distance to mars: approx 401 million km - rtt of approc 44.6 min

The internet architecture is based on the assumption that
network nodes are continuously connected. The following assumptions are valid for the terrestrial Internet architecture:
- Networks have short signal propagation delays
- Data links are symmetric and bidirectional
- Bit error rates are low

These assumptions are not valid in the space environment - a new sort of network is needed. In a space environment: 
- Connections can be routinely interrupted
- Interplanetary distances impose delays
- Link data rates are often asymmetric and some links are simplex
- Bit error rates can be very high

To communicate across these vast distances, NASA manages three communication networks consisting of distributed ground stations and space relay satellites for data transmission and reception that support both NASA and non-NASA missions. These are:
- the Deep Space Network (DSN)
- the Near Earth Network (NEN)
- the Space Network (SN). 

Communication opportunities are scheduled, based on orbit dynamics and operation plans. Sometimes a spacecraft is on the far side of a planet and you cannot communicate with it.
Transmission and reception episodes are individually configured, started and ended by command. Reliability over deeps space links is by management: on loss of data, command retransmission. More recently for Mars missions we have managed forwarding through relay points so that data from these surface vehicles is relayed thought Odyssey and other orbiting mars vehicles.


