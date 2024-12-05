# The Licklider Transfer Protocol Configuration File

Given to ltpadmin as a file or from the command line, this file configures the LTP engine itself. We will assume the local IPN node number is 1; in ION, node numbers are used as the LTP engine numbers.

## Initialize the LTP engine

````
1 32    
````

`1` refers to this being the initialization or ''first'' command.

`32` is an estimate of the maximum total number of LTP ''block'' transmission sessions - for all spans - that will be concurrently active in this LTP engine. It is used to size a hash table for session lookups.

## Defines an LTP engine 'span'

````
a span 1 32 32 1400 10000 1 'udplso localhost:1113'
````

`a` indicates that this will add something to the engine.

`span` indicates that an LTP span will be added.

`1` is the engine number for the span, the number of the remote engine to which LTP segments will be transmitted via this span. In this case, because the span is being configured for loopback, it is the number of the local engine, i.e., the local node number.

`32` specifies the maximum number of LTP ''block'' transmission sessions that may be active on this span. The product of the mean block size and the maximum number of transmission sessions is effectively the LTP flow control ''window'' for this span: if it's less than the bandwidth delay product for traffic between the local LTP engine and this spa's remote LTP engine then you'll be under-utilizing that link. We often try to size each block to be about one second's worth of transmission, so to select a good value for this parameter you can simply divide the span's bandwidth delay product (data rate times distance in light seconds) by your best guess at the mean block size.

The second `32` specifies the maximum number of LTP ''block'' reception sessions that may be active on this span. When data rates in both directions are the same, this is usually the same value as the maximum number of transmission sessions.

`1400` is the number of bytes in a single segment. In this case, LTP runs atop UDP/IP on ethernet, so we account for some packet overhead and use 1400.

`1000` is the LTP aggregation size limit, in bytes. LTP will aggregate multiple bundles into blocks for transmission. This value indicates that the block currently being aggregated will be transmitted as soon as its aggregate size exceeds 10000 bytes.

`1` is the LTP aggregation time limit, in seconds. This value indicates that the block currently being aggregated will be transmitted 1 second after aggregation began, even if its aggregate size is still less than the aggregation size limit.

````'udplso localhost:1113'```` is the command used to implement the link itself. The link is implemented via UDP, sending segments to the localhost Internet interface on port 1113 (the IANA default port for LTP over UDP).


## Starts the ltp engine itself

````
s 'udplsi localhost:1113'
````

`s` starts the ltp engine.

`'udplsi localhost:1113'` is the link service input task. In this case, the input ''duct' is a UDP listener on the local host using port 1113.


## The final configuration file - `host1.ltprc` 


````
## begin ltpadmin
1 32
a span 1 32 32 1400 10000 1 'udplso localhost:1113'
s 'udplsi localhost:1113'
## end ltpadmin
````
