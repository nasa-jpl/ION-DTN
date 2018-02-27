# JHU/APL
# Description: library file for utility functions related to parsing 
# Modification History:
#   YYYY-MM-DD     AUTHOR            DESCRIPTION
#   ----------     ---------------   -------------------------------
#   2017-11-30     Sarah             File creation
#   2018-01-02	   Evana			 Added functionality for tables
#   2018-01-05	   David			 Added new method for creating parameterized mids 
#
#############################################################################

import json
import re

import campsettings

#
# Validates if the file with the passed filename
# is a validJSON file
# Returns True if so, False if not
#
def is_jsonFile(filename):
	try:
		myjson = open(filename,'r')
	except IOError, e:
		print "[ ERROR ] Could not open file ", filename
		print e
		return False

	try:
		json_object=json.load(myjson)
	except ValueError, e:
		print "[ ERROR ] JSON file ", myjson.name, " is invalid. See error(s) below:"
		print e
		return False
	finally:
		myjson.close()

	return True

#
# Returns True if the passed string, myjson, is valid
# JSON, False if not
#
def is_jsonString(myjson):
	try:
		json_object=json.loads(myjson)
	except ValueError, e:
		print "JSON string is invalid. See error(s) below:"
		print e
		return False
	return True

#
# creates the mid for the data object
# nn is the nickname
# type is the data type 
# enum is the enumartaion of the object in the json
# data is the data object
#
# TODO: This doesn't work for CONST type. (no entry for CONST in nn_types, why?)
#
def convert_mid(nn, type, enum, data):

	# how to convert mids from the new convection of the json 
	# know the iss tag 

	types = {"EDD":"0", "VAR":"1", "RPT":"2", "CTRL":"3", "SRL":"4",
		 "TRL":"5", "MACRO":"6", "CONST":"7", "META":"7", "OP":"8", "TBL":"9"}

	nn_types = {"META":"0", "EDD":"1", "VAR":"2", "RPT":"3", "CTRL":"4",
		 "LIT":"5", "MACRO":"6", "OP":"7", "TBL":"8"}
	
	amp_mid = "0x"

	# since there will be no issuers or tags present and all the mids are compressed 
 	# only have to worry about if it is parameterized 
	flag = "8"  
	if data.has_key("parmspec"):
		flag = "c"

	try:
		# new way type byte nibble first 
		if campsettings.new_way:
			amp_mid += types[type] + flag
		# old way flag nibble first
		else: 
			amp_mid += flag + types[type]

		# nicknames
		amp_mid += "{:02x}".format(nn + int(nn_types[type]))	 
	except KeyError, e:
		print "[ ERROR ] ", type, " is not supported by convert_mid function"
		raise

	# converting the enumeration to hex and adding the length
	if enum < 255:
		amp_mid += "01" + ("{:02x}".format(enum))
	else:# if greater than 255 it takes to bytes to encode
		amp_mid += "02" + ("{:04x}".format(enum))

	return amp_mid

def create_parm_mid(mid, parmspec):
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

#
# Splits the parameters in the passed string paramspec, and
# returns them in a list format
#
def parse_params(paramspec):
	hold = paramspec.split(",")
	params = []
	for h in hold:
		params.append(h.split(":"))  
	return params

def parse_columns(columns):
	hold = columns.split(",")
	column = []
	for i in hold:
		columns.append(i.split(":"))
	return column

