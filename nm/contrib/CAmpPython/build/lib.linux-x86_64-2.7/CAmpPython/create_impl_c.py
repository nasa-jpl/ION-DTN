# JHUAPL
# Description: Creates the c file for the implementation version of the adm

# Modification History:
#   YYYY-MM-DD   AUTHOR         DESCRIPTION
#   ----------   ------------   ---------------------------------------------
#   2017-08-10   David          First implementation
#   2017-11-15   Sarah          Made long strings more readable, added error checks
#   2017-12-28   David 			Metadata is in constants now
#   2017-12-19   Evana			Insert table functionality		
##################################################################### 

import os
import re
import errno

import campch
import camputil

#
# Constructs the name and filename for this generated file.
# Returns a tuple in that order
#
# Also creates the agent directory in the out directory if
# it doesn't exist
#
# metadata is the metadata entry in the dictionary built from the JSON file
# outpath is the path to the output directory
# 
def initialize_names(metadata, outpath):
	name = "adm_" + re.sub('\s','_', metadata[0]["value"]).lower()

	# Make the agent dir, only want to allow the 'directory exists' error
	try:
		os.mkdir(outpath + "/agent/")
	except OSError as ose:
		if (not ose.errno == errno.EEXIST):
			print "[ Error ] Failed to create subdirectory in ", outpath
			print ose
			exit(-1)
	
	filename = outpath + "/agent/" + name + "_impl.c"
	return name, filename

#
# Writes the metadata functions to the file passed
# new_c is an open file descriptor to write to
# name is the value returned from initialize_names()
# metadata is a list of metadata to include
#
def write_metadata_functions(new_c, name, metadata):
	new_c.write("\n/* Metadata Functions */\n\n")
	
	metadata_funct_str = (
		"\nvalue_t {0}_meta_{1}(tdc_t params)"
		"\n{{"
		"\n\treturn val_from_str(\"{2}\");"
		"\n}}"
		"\n\n")

	new_c.write(metadata_funct_str.format(name, "name", name))
	
	for i in metadata[1:]:
		try:
			new_c.write(metadata_funct_str.format(name, i["name"].lower(), i["value"]))
		except KeyError, e:
			print "[ Error ] Badly formatted metadata. Key not found:"
			print e
			raise

			
#
#writes the table functions to the file passed
#new_c is an open file descriptor to write to
#name is the value returned from initialize_names()
#table is a list of tables to include
#

def write_table_functions(new_c,name,table, custom):
	new_c.write("\n/* Table Functions */\n\n")
	
	function_name_template = "tbl_{}"

#the information being printed out about the tables
#is split into three pieces
	table_function_begin_str = (
		"\n{0}"
		"\n{1}"
		"\ntable_t* {2}_{3}()"
		"\n{{"
		"\n\ttable_t *table = NULL;"
		"\n\tif((table = table_create(NULL,NULL)) == NULL)"
		"\n\t{{"
		"\n\t\treturn NULL;"
		"\n\t}}\n\n")
	table_function_mid_str = (
		"\n\t\t(table_add_col(table, \"{1}\", AMP_TYPE_{0}) == ERROR) ||")
	table_function_body_str = (
		"\n\t{"
		"\n\t\ttable_destroy(table, 1);"
		"\n\t\treturn NULL;\n\t}\n")
	for i in table:
#format this function's name, description, columns
		try:
			function_name = function_name_template.format(i["name"].lower())
			description = campch.multiline_comment_format(i["description"])
			# column_str = str(campch.multiline_comment_format(i["columns"]))
			new_c.write(table_function_begin_str.format(description,"",name,function_name))
			columns = i["columns"]
			if columns != []:
				count=0
				length=len(columns)
				new_c.write("\tif(")
				length-=1
				for pair in columns:
					for key in pair:
						value = pair[key]
						if count==length:
							new_c.write("\n\t\t(table_add_col(table, \""+value+"\", AMP_TYPE_"+key+") == ERROR))")
						else:
							new_c.write(table_function_mid_str.format(key,value))
						count+=1
			new_c.write(table_function_body_str)
			new_c.write("\n")
		except KeyError, e:
			print "[ Error ] Badly formatted table. Key not found:"
			print e
			raise
		
		# Add custom body tags and any scrapped lines found
		campch.write_function_custom_body(new_c, function_name, custom.get(function_name, []))

		# Close out the function
		new_c.write("\treturn table;\n}\n\n")

#
# Writes the edd functions to the file passed
# new_c is an open file descriptor to write to
# name is the value returned from initialize_names()
# edds is a list of edds to include
# custom is a dictionary of custom function bodies to include, in the
# form {"function_name":["line1", "line2",...], ...}
#
def write_edd_functions(new_c, name, edds, custom):
	new_c.write("\n/* Collect Functions */")

	function_name_template = "get_{}"
	edd_function_begin_str = (
		"\n{0}"
		"\nvalue_t {1}(tdc_t params)"
		"\n{{"
		"\n\tvalue_t result;"
		"\n")

	for i in edds:
		# Format _this_ function's name, and write beginning of function to file
		try:
			function_name = function_name_template.format(i["name"].lower())
			description = campch.multiline_comment_format(i["description"])
			new_c.write(edd_function_begin_str.format(description, name+"_"+function_name))
		except KeyError, e:
			print "[ Error ] Badly formatted edd. Key not found:"
			print e
			raise
		
		# Add custom body tags and any scrapped lines found
		campch.write_function_custom_body(new_c, function_name, custom.get(function_name, []))

		# Close out the function
		new_c.write("\treturn result;\n}\n\n")

#
# Writes the control functions to the file passed
# new_c is an open file descriptor to write to
# name is the value returned from initialize_names
# controls is a list of controls to include
# custom is a dictionary of custom function bodies to include, in the
# form {"function_name":["line1", "line2",...], ...}
#
def write_control_functions(new_c, name, controls, custom):
	new_c.write("\n\n/* Control Functions */\n")

	function_name_template = "ctrl_{}"
	ctrl_function_begin_str = (
		"\n{0}"
		"\ntdc_t* {1}(eid_t *def_mgr, tdc_t params, int8_t *status)"
		"\n{{"
		"\n\ttdc_t* result = NULL;"
		"\n\t*status = CTRL_FAILURE;"
		"\n")

	for i in controls:
		try:
			# Format this function's name and write the beginning of the function to file
			function_name = function_name_template.format(i["name"].lower())
			description = campch.multiline_comment_format(i["description"])
			new_c.write(ctrl_function_begin_str.format(description, name+"_"+function_name))
		except KeyError, e:
			print "[ Error ] Badly formatted control. Key not found:"
			print e
			raise

		campch.write_function_custom_body(new_c, function_name, custom.get(function_name, []))

		new_c.write("\treturn result;\n}\n\n")

#
# Writes the operator functions to the file passed
# new_c is an open file descriptor to write to
# name is the value returned from initialize_names
# operators is a list of operators to include
# custom is a dictionary of custom function bodies to include, in the
# form {"function_name":["line1", "line2",...], ...}
#
def write_operator_functions(new_c, name, operators, custom):
	new_c.write("\n\n/* OP Functions */\n")

	function_name_template = "op_{}"
	op_function_begin_str = (
		"\n{0}"
		"\nvalue_t {1}(Lyst stack)"
		"\n{{"
		"\n\tvalue_t result;"
		"\n")
	
	for i in operators:
		try: 
			function_name = function_name_template.format(i["name"].lower())
			description = campch.multiline_comment_format(i["description"])
			new_c.write(op_function_begin_str.format(description, name+"_"+function_name))
		except KeyError, e:
			print "[ Error ] Badly formatted operator. Key not found:"
			print e
			raise
		
		campch.write_function_custom_body(new_c, function_name, custom.get(function_name, []))

		new_c.write("\treturn result;\n}\n\n")

#
# function for writting the setup function and 
# custom body if present 
#
# body: the scraped body of the setup function
#
def write_setup(new_c, name, body):
	new_c.write("void "+name+"_setup(){\n\n")
	campch.write_function_custom_body(new_c, "setup", body.get("setup", []))
	new_c.write("}\n\n")

#
# function for writting the cleanup function and 
# custom body if present 
#
# body: the scraped body of the setup function
#
def write_cleanup(new_c, name, body):
	new_c.write("void "+name+"_cleanup(){\n\n")
	campch.write_function_custom_body(new_c, "cleanup", body.get("cleanup", []))
	new_c.write("}\n\n")





#
# Main function of this file, which calls helper functions to
# orchestrate the creation of the generated file
#
# data: the dictionary made from a parsed JSON file
# outpath: the output directory
#
def create(data, out, scrape_file):
	try:
		metadata = data["adm:constants"]
		name, filename = initialize_names(metadata, out)
	except KeyError, e:
		print "[ Error ] JSON does not include valid metadata"
		print e
		return

	# Scrape the previous file if included
	includes = []
	custom_func = []
	func_bods = {}
	if scrape_file is not None:
		print "Scraping ", scrape_file, " ... ",
		includes, custom_func, func_bods = campch.scrape(scrape_file)
		print "\t[ DONE ]"
		
		# Sanity Check. If scraping was requested and returned nothing, let the user know
		if (len(includes) == 0 and len(custom_func) == 0 and len(func_bods) == 0):
			print "\t[ Warning ] No custom input found to scrape in ", scrape_file
	else:
		includes = ["/*             TODO              */\n"]
		custom_func = ["/*             TODO              */\n"]
			
	# Create the new file
	try:
		new_c = open(filename,"w")
	except IOError, e:
		print "[ Error ] Failed to open ", filename, " for writing."
		print e
		return		

	print "Working on ", filename,
			
	campch.write_standard_c_file_header(new_c, filename)

	# custom includes tag
	campch.write_custom_includes(new_c, includes)
	new_c.write(campch.make_include_str(["{}_impl.h".format(name)]))
	
	
	# Any custom functions scraped
	campch.write_custom_functions(new_c, custom_func)
	
	write_setup(new_c, name, func_bods)
	write_cleanup(new_c, name, func_bods)
	# metadata, edd, control, and operator functions
	try:
		write_metadata_functions(new_c, name, data.get("adm:constants", [])[:4])
		write_table_functions(new_c,name,data.get("adm:tables",[]), func_bods)
		write_edd_functions(new_c, name, data.get("adm:externally-defined-data", []), func_bods)
		write_control_functions(new_c, name, data.get("adm:controls", []), func_bods)	
		write_operator_functions(new_c, name, data.get("adm:operators", []), func_bods)
	except KeyError, e:
		return
	finally:
		new_c.close()

	print "\t[ DONE ]"
