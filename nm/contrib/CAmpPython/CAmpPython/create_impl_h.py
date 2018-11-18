# JHU/APL
# Description: Creates the h file for the implementation version of the adm
# Modification History:
#   YYYY-MM-DD    AUTHOR         DESCRIPTION
#   ----------    ------------   ---------------------------------------------
#   2017-08-09    David          First implementation
#   2017-11-01    Sarah          Making long strings easier to parse
#   2017-11-20    Evana		 Updates
#	2017-12-28    David 			Metadata is in constants now
#
##################################################################### 

import re
import os
import errno

import campch

#
# Constructs the name and filename for this generated file. Returns as a
# tuple in that order
#
# Also creates the agent directory in the out directory if it doesn't
# exist.
#
# metadata is metadata entry in the dictionary built from the JSON file
# outpath is the path to the output directory
#
def initialize_names(metadata, outpath):
	name = "adm_" + re.sub('\s','_',metadata[0]["value"]).lower()

	# Make the agent dir, only want to allow the 'directory exists' error
	try:
		os.mkdir(outpath + "/agent/")
	except OSError as ose:
		if (not ose.errno == errno.EEXIST):
			print "[ Error ] Failed to create subdirectory in ", outpath
			print ose
			exit(-1)
		
	filename = outpath + "/agent/" + name + "_impl.h"
	return name, filename

#
# Writes the #defines and ifdefs to the file
# new_h is an open file descriptor to write to
# name is the name returned from initialize_names()
#
def write_defines(new_h, name):
	n_upper = name.upper()
	define_str = (
		"#ifndef {0}_IMPL_H_\n"
		"#define {0}_IMPL_H_\n\n")
	new_h.write(define_str.format(n_upper))

#
# Writes the #includes for the file
# new_h is an open file descriptor to write to
#
def write_includes(new_h):
	files = ["../shared/primitives/tdc.h", "../shared/primitives/value.h", 
		 "../shared/utils/utils.h",    "../shared/primitives/ctrl.h", 
		 "../shared/primitives/table.h" ]
	
	new_h.write(campch.make_include_str(files))

#
# Writes the metadata functions to the passed new_h file
# name is the name returned from initialize_names()
#
def write_metadata_functions(new_h, name):
	meta_str = (
		"/* Metadata Functions */"
		"\nvalue_t {0}_meta_name(tdc_t params);"
		"\nvalue_t {0}_meta_namespace(tdc_t params);\n"
		"\nvalue_t {0}_meta_version(tdc_t params);\n"
		"\nvalue_t {0}_meta_organization(tdc_t params);\n"
		"\n")
	new_h.write(meta_str.format(name))

#
# Writes the edd collect functions to the passed file, new_h
# name is the value returned from initialize_names
# edds is a list of the edds to include
#
def write_collect_functions(new_h, name, edds):
	new_h.write("\n/* Collect Functions */\n")
	function_str = "value_t {0}_get_{1}(tdc_t params);\n"
	for i in edds:
		try:
			new_h.write(function_str.format(name, i["name"].lower()))
		except KeyError, e:
			print "[ Error ] Badly formatted edd. Key not found:"
			print e
			raise

#
# Writes the control functions to the passed file, new_h
# name is the value returned from initialize_names
# controls is a list of the controls to include
#
def write_control_functions(new_h, name, controls):
	new_h.write("\n\n/* Control Functions */\n")
	function_str = "tdc_t* {0}_ctrl_{1}(eid_t *def_mgr, tdc_t params, int8_t *status);\n"
	for i in controls:
		try:
			new_h.write(function_str.format(name, i["name"].lower()))
		except KeyError, e:
			print "[ Error ] Badly formatted control. Key not found:"
			print e
			raise

#
# Writes the operator functions to the passed file, new_h
# name is the value returned from initialize_names
# operators is a list of operators to include
#
def write_operator_functions(new_h, name, operators):
	new_h.write("\n\n/* OP Functions */\n")
	function_str = "value_t* {0}_op_{1}(Lyst stack);\n"
	for i in operators:
		try: 
			new_h.write(function_str.format(name, i["name"].lower()))
		except KeyError, e:
			print "[ Error ] Badly formatted operator. Key not found:"
			print e
			raise
		
# 
# Main function of this file, which calls helper functions to orchestrate
# the creation of the generated file
#
# data: the dictionary built from the parsed JSON file
# outpath: the output directory
#
def create(data, outpath, scrape_file):
	
	# valid metadata is required
	try:
		metadata = data["adm:constants"]
		name, filename = initialize_names(metadata, outpath)
	except KeyError, e:
		print "[ Error ] JSON does not include valid metadata"
		print e
		return

	# Scrape the previous file if included
	includes = ["/*             TODO              */\n"]
	custom_func = ["/*             TODO              */\n"]
	type_enums = ["/*             TODO              */\n"]
	if scrape_file is not None:
		print "Scraping ", scrape_file, " ... ",
		includes, custom_func, type_enums = campch.scrapeH(scrape_file)
		print "\t[ DONE ]"
		# Sanity Check. If scraping was requested and returned nothing, let the user know
		if (len(includes) == 0 and len(custom_func) == 0):
			print "\t[ Warning ] No custom input found to scrape in ", scrape_file
			
	try:
		new_h = open(filename,"w")
	except IOError, e:
		print "[ Error ] Failed to open ", filename, " for writing."
		print e
		return

	print "Working on ", filename,
	
	campch.write_standard_h_file_header(new_h, filename)

	write_defines(new_h, name)

	campch.write_custom_includes(new_h, includes)

	write_includes(new_h)
	
	campch.write_type_enums(new_h, type_enums)


	# init agent function
	new_h.write("void {}_adm_init_agent();\n\n\n".format(re.sub('\s','_', metadata[0].get("name", ""))))

	# Write this to divide up the file
	new_h.write("/******************************************************************************"+
                    "\n *                            Retrieval Functions                             *"+
                    "\n ******************************************************************************/"+
		    "\n\n")
	
	# Write any custom functions found
	campch.write_custom_functions(new_h, custom_func)

	#the setup and clean up functions
	new_h.write("void "+name+"_setup();\n")
	new_h.write("void "+name+"_cleanup();\n\n")

	write_metadata_functions(new_h, name)

	try: 
		write_collect_functions(new_h, name, data.get("adm:externally-defined-data", []))
		write_control_functions(new_h, name, data.get("adm:controls", []))
		write_operator_functions(new_h, name, data.get("adm:operators", []))
	except KeyError, e:
		return
	finally:
		new_h.write("\n#endif //{0}_IMPL_H_\n".format(name.upper()))	
		new_h.close()

	print "\t[ DONE ]"
