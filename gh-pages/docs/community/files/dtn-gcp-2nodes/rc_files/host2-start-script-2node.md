# ION Start Script Example
Note: place this in a file named host2.rc

````

## begin ionadmin 
1 2 ''
s

a contact +1 +3600 1 1 100000
a contact +1 +3600 1 2 100000
a contact +1 +3600 2 1 100000
a contact +1 +3600 2 2 100000 

a range +1 +3600 1 1 1
a range +1 +3600 1 2 1
a range +1 +3600 2 2 1

m production 1000000
m consumption 1000000
## end ionadmin 

## begin ltpadmin 
1 32

a span 1 10 10 1400 10000 1 'udplso `external_IP_of_node_1`:1113'
a span 2 10 10 1400 10000 1 'udplso `external_IP_of_node_2`:1113'
s 'udplsi `internal_IP_of_node_2`:1113'
## end ltpadmin 

## begin bpadmin 
1
a scheme ipn 'ipnfw' 'ipnadminep'

a endpoint ipn:2.0 q
a endpoint ipn:2.1 q
a endpoint ipn:2.2 q

a protocol ltp 1400 100
a induct ltp 2 ltpcli
a outduct ltp 2 ltpclo
a outduct ltp 1 ltpclo

s
## end bpadmin 

## begin ipnadmin 
a plan 1 ltp/1
a plan 2 ltp/2
## end ipnadmin

## begin ionsecadmin
1
## end ionsecadmin

````
