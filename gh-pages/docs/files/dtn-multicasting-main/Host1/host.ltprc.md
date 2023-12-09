Note: place this in a file named host.ltprc

````
1 32
#a span peer_engine_nbr
#       [queuing_latency]
#       Create a span to tx
a span 1 32 32 1400 10000 1 'udplso external_ip_host1:1113' 300
a span 2 32 32 1400 10000 1 'udplso external_ip_host2:1113' 300
#       Start listening for incoming LTP traffic - assigned to the IP internal
s 'udplsi internal_ip:1113'
## end ltpadmin
````
