Note: place this in a file named host.ionrc

````
## begin ionadmin
1 1 ''
s
#       Define contact plan
a contact +1 +3600 1 1 100000
a contact +1 +3600 1 2 100000
a contact +1 +3600 2 1 100000
a contact +1 +3600 2 2 100000

#       Define 1sec OWLT between nodes
a range +1 +3600 1 1 1
a range +1 +3600 1 2 1
a range +1 +3600 2 1 1
a range +1 +3600 2 2 1
m production 1000000
m consumption 1000000
## end ionadmin
````
