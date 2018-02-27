#!/usr/bin/env python

# JHU/APL
# Description: Entrypoint into the CAmpPython program. Creates c and h files
# from JSON of different ADMs for use by protocols in DTN
#
# Modification History:
#   YYYY/MM/DD	    AUTHOR	   DESCRIPTION
#   ----------	  ------------	 ---------------------------------------------
#   2017-08-10	  David		 First implementation
#   2017-08-15	  Evana		 Clean up
#   2017-08-17	  David		 fixed mid converter
#   2017-08-23	  David		 Started convertion to command line tool
#   2017-11-01	  Sarah		 Documentation and minor fixups
##################################################################### 


import os
import argparse
import json
import re

import create_gen_h
import create_agent_c
import create_mgr_c
import create_impl_h
import create_impl_c
import create_step4

import camputil
import campsettings

#
# Sets up the format of command line arguments for CAmpPython,
# validates and returns arguments input on the command line
#
# Return value is a tuple (args, jsonfilename)
#
def verify_command_line_arguments():
	p = argparse.ArgumentParser(description = 'C code generator for Asychronous management protocols')

	# camp [-o PATH] [-s FILE] [-b B] [-n N] [-u] json
	
	p.add_argument('-o', '--out',         help="The ouput directory",                          default="./")
	p.add_argument('-c', '--scrapeC',      help="Previously generated c file to be scraped",    default=None)
	p.add_argument('-s', '--scrapeH',      help="Previously generated h file to be scraped",    default=None)
	p.add_argument('-b', '--firstByte',   help="The order of the first byte of the MID",       default="old")
	
	p.add_argument('-n', '--nickname',    help="The integer nickname to use for this file",    default=-1, type=int)
	p.add_argument('-u', '--update-nn',   help="Flag set to update nickname in name registry", action='store_true')
	
	p.add_argument('json',                help="JSON file to use for file generation")

	args = p.parse_args()	
	filename = args.json
	
	# Make sure the passed file is valid JSON
	if (not camputil.is_jsonFile(filename)):
		print "[ ERROR ] ", filename, " is not a valid JSON file\n"
		raise Exception

	return args, filename

#
# Makes the output directory if it doesn't already exist
# d_name is the path to the output directory
#
def set_up_outputdir(d_name):
	if(not os.path.isdir(d_name)):
		try:
			os.makedirs(d_name)
		except OSError, e:
			print "[ Error ] Failed to make output directory\n",
			print e
			raise

#
# Finds the integer value for this JSON type in the name_registry file if it exists
# cname is the colon-deliminated name in the JSON metadata
# returns the integer value if found, < 0 if not found.
#
def parse_nickname(cname):
	name_registry_fn = os.path.join((os.path.abspath(os.path.dirname(__file__))), 'data', 'name_registry.txt')
	for line in open(name_registry_fn).readlines():
		nn = re.findall('(arn:\S+)=(\d+)',line.strip())
		if nn[0][0] == cname:
			return int(nn[0][1])

	return -1

#
# Adds the passed name and nickname to the name_registry.txt file
#
def add_to_registry(name, passed_nn):
	name_registry_fn = os.path.join((os.path.abspath(os.path.dirname(__file__))), 'data', 'name_registry.txt')
	
	try:		
		with open(name_registry_fn, 'a') as nr_file:
			print "\tAdding to name registry: ", name, "=", str(passed_nn)
			nr_file.write(name.strip() + "=" + str(passed_nn).strip() + "\n")

	except IOError, e:
		print "\t[ Warning ] Unable to update name_registry.txt. Passed value will still be used for file generation."
		print "\t"+str(e)
		

#
# Updates the passed name in the name registry with the passed value
#
def update_registry(name, passed_nn):
	name_registry_fn = os.path.join((os.path.abspath(os.path.dirname(__file__))), 'data', 'name_registry.txt')
	nicknames = {}

	# Read all into dictionary
	for line in open(name_registry_fn).readlines():
		key, value = line.split("=")
		nicknames[key] = value
		
	try:		
		# Write back to file, replacing this one
		nr_file = open(name_registry_fn, 'w')
		for key, value in nicknames.items():
			if(key == name):
				print "\tUpdating name registry: ", key, "=", value.strip(), "->", str(passed_nn)
				value = str(passed_nn).strip() + "\n"
			nr_file.write(key + "=" + value)
		nr_file.close()
		
	except IOError, e:
		print "\t[ Warning ] Unable to update name_registry.txt. Passed value will still be used for file generation."
		print "\t"+str(e)
	
#
# Resolves any discrepancies between nn read from name_registry
# and one passed via command line. Favors command line over
# registry. If u_flag is set and they differ or the name is not
# present in the registry, updates name registry.
#
def choose_and_update_nickname(name, read_nn, passed_nn, u_flag):

	# if nn was not passed via command line or given in name_registry.txt, error and exit
	if(read_nn == -1 and passed_nn == -1):
		print "[ Error ] nickname integer value must be passed via command line or set in name_registry.txt"
		exit(-1)
		
	# if value only present in name_registry, use it
	elif (passed_nn == -1):
		return read_nn
	
	# if value only present on command line, use it and optionally add to registry
	elif (read_nn == -1):
		if u_flag:
			add_to_registry(name, passed_nn)
		return passed_nn
	
	# if both are set, use passed value and optionally update registry
	else:
		if u_flag:
			update_registry(name, passed_nn)
		return passed_nn

#
# Main function of CAmpPython
#
# Calls helper functions to initialize settings, read command line arguments,
# parse the input JSON file, and generate c and h files
#
def main():
	try:
		options, json_filename = verify_command_line_arguments()

		# Initialize settings and output dir with the options returned 
		campsettings.init(options.firstByte)
		set_up_outputdir(options.out)

		passed_nn = options.nickname
	except Exception, e:
		return -1

	# Parse the JSON file
	print "Parsing " + json_filename + " ...",
	
	jfile = open(json_filename, 'r')
	myjson = json.load(jfile)
	jfile.close()
	
	print "\t[ DONE ]"

	# Check the name registry for any nickname
	try:
		cname = myjson["adm:constants"][1]["value"]
	except KeyError, e:
		print "[ Error ] Badly formatted JSON, missing key:"
		print e
		return
	read_nn = parse_nickname(cname)
	nn = choose_and_update_nickname(cname, read_nn, passed_nn, options.update_nn)
	
	# Generate files for the JSON
	print "Generating files ..."
	create_impl_h.create(myjson, options.out, options.scrapeH)
	create_impl_c.create(myjson, options.out, options.scrapeC)
	
	create_gen_h.create(myjson, options.out, nn)
	create_mgr_c.create(myjson, options.out)
	create_agent_c.create(myjson, options.out)
	# create_step4.create(myjson,options.out)

	print "[ End of CAmpPython Execution ]\n"

	


if __name__ == '__main__':
	main()	
