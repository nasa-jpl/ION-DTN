# DTNfog

DTNfog is a program that aims to implement a fog architecture in a DTN environment. The DTNfog node receives a .tar file containing a command and a data file from a node of lower hierarchical level. If the DTNfog node is able to execute the command on the data file it gives the response back, otherwise it forwards the request to the upper DTNfog node, and so on. Archive files can be transferred either by TCP or by Bundle Protocol. DTNfog has been designed for challenged networks, including a future Interplanetary Internet


DTNfog is part of the DTNsuite, which consists of several DTN applications making use of the Abstraction Layer, plus the Abstraction Layer itself


Being built on top of the Abstraction layer, DTNfog is compatible with all the most important bundle protocol implementations (DTN2, ION, IBR_DTN)

## Guide
Please refer to the DTNfog .pdf guide for further information on
- release notes
- building instructions
- a brief overview and a concise user guide.

## Authors
- Lorenzo Tullini (lorenzo.tullini@studio.unibo.it)
- Carlo Caini (Academic supervisor, carlo.caini@unibo.it)
