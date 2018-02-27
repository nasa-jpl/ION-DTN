
def create_mid(mid, parmspec):
	types = []
	values = []
	type_enum = {'BYTE':9, 'INT':10, 'UINT':11, 'VAST':12, 'UVAST':13, 'REAL32':14, 'REAL64':15, 'SDNV':16, 'TS':17, 'STR':18}
	
	for i in parmspec:
		for key in i:
			types.append(key)
			values.append(i[key])

	mid += "{:02x}".format(len(parmspec)+1)
	mid += "{:02x}".format(len(types))
	
	for i in types:
		mid +=  "{:02x}".format(type_enum[i])

	for i in values:	
		if isinstance(i,str):
			v = i.encode("hex")
		else:
			v = "{:x}".format(i)
		v = str(v)
		
		if len(v) % 2 != 0:
			v = "0" + v
		length = str("{:x}".format(len(v)/2))
		
		if len(length) % 2 != 0:
			length = "0" + length

		mid += length + v

	return mid 

	
parms = [{"INT":10}, {"INT":17}]

print create_mid("0xc3510100",parms)
