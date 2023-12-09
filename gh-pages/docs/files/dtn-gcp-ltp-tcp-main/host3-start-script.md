# ION Start Script Example
Note: place this in a file named host3.rc

````

## begin ionadmin 
# ionrc configuration file for host3 in a 3node tcp/ltp test.
# This uses tcp from 1 to 3.
# 
# Initialization command (command 1). 
# Set this node to be node 3 (as in ipn:3).
# Use default sdr configuration (empty configuration file name '').
1 3 ''
# start ion node
s
# Add a contact.
# It will start at +1 seconds from now, ending +3600 seconds from now.
# It will connect node 3 to itself
# It will transmit 100000 bytes/second.
a contact +1 +3600 3 3 100000

# Add more contacts.
# They will connect 2 to 3, 3 to 2, and 3 to itself
# Note that contacts are unidirectional, so order matters.
a contact +1 +3600 3 2 100000
a contact +1 +3600 2 3 100000
a contact +1 +3600 2 2 100000

# Add a range. This is the physical distance between nodes.
a range +1 +3600 3 3 1

# Add more ranges.
a range +1 +3600 2 2 1
a range +1 +3600 2 3 1

# set this node to consume and produce a mean of 1000000 bytes/second.
m production 1000000
m consumption 1000000
## end ionadmin 

## begin bpadmin 
# bprc configuration file for host3 in a 3node test.
# Initialization command (command 1).
1

# Add an EID scheme.
a scheme ipn 'ipnfw' 'ipnadminep'

# Add endpoints.
a endpoint ipn:3.0 q
a endpoint ipn:3.1 q
a endpoint ipn:3.2 q

# Add a protocol. 
# Add the protocol named tcp.
a protocol tcp 1400 100

# Add an induct. (listen)
a induct tcp 10.0.0.3:4556 tcpcli

# Add an outduct (send to yourself).
a outduct tcp 10.0.0.3:4556 tcpclo

# Add an outduct. (send to host2)
a outduct tcp external_ip_of_host_2:4556 tcpclo

# Start bundle protocol engine, also running all of the induct, outduct,
# and administration programs defined above.
s
## end bpadmin 

## begin ipnadmin 
# ipnrc configuration file for host1 in the 3node tcp network.
# Add an egress plan (to yourself).
a plan 2 tcp/10.0.0.3:4556
# Add an egress plan (to the host 2).
a plan 2 tcp/external_IP_of_node_2:4556
## end ipnadmin

## begin ionsecadmin
1
## end ionsecadmin

````
