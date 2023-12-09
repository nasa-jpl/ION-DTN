Note: place this in a file named host.bprc.1

````
## begin bpadmin
1
#       Use the ipn eid naming scheme
a scheme ipn 'ipnfw' 'ipnadminep'
#       Create a endpoints
a endpoint ipn:1.0 q
a endpoint ipn:1.1 q
a endpoint ipn:1.2 q
#       Define ltp as the protocol used
a scheme imc 'imcfw' 'imcadminep'
#a endpoint imc:19.0 q
a protocol ltp 1400 100
#       Listen
a induct ltp 1 ltpcli
#       Send to yourself
a outduct ltp 1 ltpclo
#       Send to server2
a outduct ltp 2 ltpclo
a outduct ltp 3 ltpclo
w 1
s
## end bpadmin
````