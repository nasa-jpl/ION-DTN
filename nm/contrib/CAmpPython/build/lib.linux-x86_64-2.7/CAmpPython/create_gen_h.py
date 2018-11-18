# JHU/APL
# Description: This creates the h file for the public portions of an adm
# Modification History:
# YYYY-MM-DD  AUTHOR     DESCRIPTION
# ----------  ---------- -----------------------------
# 2017-08-10  Evana       
#	      David      cleaning up
#	      Evana      fix formatting
# 2017-08-14  Evana      cleaning up
# 2017-08-15  Evana	 fix formatting
# 2017-08-15  David	 Added AMP mids
# 2017-08-17  David      fixed midconverter
# 2017-08-22  Evana      cleaning up
# 2017-11-15  Sarah      error handling and cleaning up
# 2017-12-27  David  change constants to require types 
# 2017-12-28     David 			Metadata is in constants now
########################################################

import re
import os 
import errno

import camputil
import campch

nickname = -1
#
# Constructs the name, shortname, colon-deliminated name and filename for
# this generated file. Returns as a tuple in that order.
#
# Also creates the shared directory in the out directory
# if it doesn't exist.
#
# metadata is the metadata entry in the dictionary built from the JSON file
# outpath is the path to the output directory
#
def initialize_names(metadata, outpath):
	short_name = re.sub('\s','_',metadata[0]["value"]).lower()
	name       = "adm_" + short_name
	cname      = metadata[1]["value"]

	# Make the shared dir, only want to allow the 'directory exists' error
	try:
		os.mkdir(outpath + "/shared/")
	except OSError as ose:
		if(not ose.errno == errno.EEXIST):
			print "[ Error ] Failed to create subdirectory in ", outpath
			print ose
			exit(-1)

	filename = outpath + "/shared/" + name + ".h"
	return name, short_name, cname, filename

#
# Writes the #defines for the generated file
# h_file is an open file descriptor to write to
# name and shortname are the values returned from initialize_names
#
def write_defines(h_file, name, shortname):
	name_upper = name.upper();
	sn_upper = shortname.upper();
	defines_str = (
		"\n#ifndef {0}_H_"
		"\n#define {0}_H_"
		"\n#define _HAVE_{1}_ADM_"
		"\n#ifdef _HAVE_{1}_ADM_"
		"\n")
	h_file.write(defines_str.format(name_upper, sn_upper))

#
# Writes the includes statements for this generated file
# h_file is an open file descriptor to write to
# shortname is the shortname returned from initialize_names()
#
def write_includes(h_file, shortname):
	files = ["lyst.h",
		 #"{}_instr.h".format(shortname), 
		 "../utils/nm_types.h", "adm.h" ]

	h_file.write(campch.make_include_str(files))

#
# Writes the ADM template documentation for this generated file
# h_file is an open file descriptor to write to
# name is the colon-deliminated name returned from initialize_names()
#
def write_adm_template_documentation(h_file, name):
	documentation_str = (
		"\n/*"
		"\n * +----------------------------------------------------------------------------------------------------------+"
		"\n * |			              ADM TEMPLATE DOCUMENTATION                                              +"
		"\n * +----------------------------------------------------------------------------------------------------------+"
		"\n *"
		"\n * ADM ROOT STRING:{}"
		"\n */"
		"\n")
	h_file.write(documentation_str.format(name))

#
# Writes the agent nickname #defines to the file
#
def write_agent_nickname_definitions(h_file, short_name, nn):
	sn_upper = short_name.upper();
	comment_str = (
		"\n/*"
		"\n * +----------------------------------------------------------------------------------------------------------+"
 		"\n * |				             AGENT NICKNAME DEFINITIONS                                       +"
 		"\n * +----------------------------------------------------------------------------------------------------------+"
 		"\n */"
		"\n")
	h_file.write(comment_str)

	idx_define_str = "#define {0}_ADM_{1}_NN_IDX {2}\n"
	str_define_str = "#define {0}_ADM_{1}_NN_STR \"{2}\"\n\n"

	for idx, item in enumerate(["META", "EDD", "VAR", "RPT", "CTRL",
				    "CONST", "MACRO", "OP", "TBL", "ROOT"]):
		h_file.write(idx_define_str.format(sn_upper, item, str(nn+idx)))
		h_file.write(str_define_str.format(sn_upper, item, str(nn+idx)))
	
#
# Function for formatting the description in the tables
# wraps the description nicely and fills other cells in the
# row with empty spaces.
# descr is the string to format, returns a string result
# 
def format_table_description(descr):
	result = []
	descr = descr.strip()
	
	fill_this_row = "|%-13.13s|\n"%("")                # the rest of this row
	start_next_row = "   |%-29.29s|%-12.12s|"%("", "") # next row up until description column

	# split because descriptions may contain new lines
	lines = descr.splitlines()
	num_lines = len(lines)
	for line_idx in range(0, num_lines):
		
		# This to wrap nicely inside the column
		line = lines[line_idx].strip()
		while len(line) > 50:
			left, right = line[:50], line[50:]
			result.append('%-50.50s'%(left))
			result.append(fill_this_row)
			result.append(start_next_row)
			line = right

		result.append('%-50.50s'%(line))
		
		# If there are still lines to write, fill the row and start a new one
		if(line_idx < num_lines-1):
			result.append(fill_this_row)
			result.append(start_next_row)
			
	return "".join(result)

#
# Formats and prints a table entry to the passed file descriptor, fd, with
# the passed name, mid, description, and type (ttype)
#
def format_table_entry(fd, name, mid, description, ttype):
	description = format_table_description(description)
	entry_format = "   |%-29.29s|%-12.12s|%s|%-13.13s|\n"%(name, mid, description, ttype)
	line_format = "   +-----------------------------+------------+--------------------------------------------------+-------------+\n"
	fd.write(entry_format)
	fd.write(line_format)

#
# Makes and returns the header for a definition table, with the passed
# definitions string in the center area
#
def make_definition_table_header(definitions_str):
	header_str = (
		"\n/*"
		"\n * +-----------------------------------------------------------------------------------------------------------+"
		"\n * |		                    {}"
		"\n * +-----------------------------------------------------------------------------------------------------------+"
		"\n   +-----------------------------+------------+--------------------------------------------------+-------------+"
		"\n   |             NAME            |    MID     |              DESCRIPTION                         |     TYPE    |"
		"\n   +-----------------------------+------------+--------------------------------------------------+-------------+"
		"\n")
	return header_str.format(definitions_str)

#
# Writes the metadata definitions and #defines to the file
# h_file is an open file descriptor to write to
# name and short_name are the values returned by initialize_names
# metadata is a list of the metadata to include
#
def write_metadata_definitions(h_file, name, short_name, metadata):
	name_upper = name.upper()

	# The table comment section
	header_str = make_definition_table_header(short_name.upper() + " META-DATA DEFINITIONS %-57.57s"%(" "))
	h_file.write(header_str)

	count = 0
	for i in metadata:
		try:
			format_table_entry(h_file, i["name"], camputil.convert_mid(nickname, "META",count,i), i["description"], i["type"])
		except KeyError, e:
			print "[ Error ] Badly formatted metadata. Key not found: "
			print e
			raise
		count += 1 
	h_file.write(" */\n")

	count = 0
	# #defines
	for i in metadata:
		try:
			h_file.write("// \"{}\"\n".format(i["name"]))
			h_file.write("#define {0}_META_{1}_MID {2}\n".format(name_upper, i["name"].upper(), camputil.convert_mid(nickname, "META", count, i)))
		except KeyError, e:
			print "[ Error ] Badly formatted metadata. Key not found: "
			print 
			raise
		count += 1
		     
	h_file.write("\n")

#
# Writes the edd definitions and #defines to the file
# h_file is an open file descriptor to write to
# short_name is the value returned by initialize_names
# edds is a list of edds to include
#
def write_edd_definitions(h_file, short_name, edds):
	sn_upper = short_name.upper()

	# the table comment section
	header_str = make_definition_table_header(sn_upper+" EXTERNALLY DEFINED DATA DEFINITIONS %-46.46s"%(" "))
	h_file.write(header_str)

	count = 0
	for i in edds:
		try:
			format_table_entry(h_file, i["name"], camputil.convert_mid(nickname, "EDD", count, i), i["description"], i["type"])
		except KeyError, e:
			print "[ Error ] Badly formatted edd. Key not found: "
			print e
			raise
		count += 1

	h_file.write(" */\n")

	count = 0
	# #defines
	for i in edds:
		try:
			h_file.write("#define ADM_{0}_EDD_{1}_MID {2}\n".format(sn_upper, i["name"].upper(), camputil.convert_mid(nickname, "EDD", count, i)))
		except KeyError, e:
			print "[ Error ] Badly formatted edd. Key not found: "
			print e
			raise
		count += 1

	h_file.write("\n")

#
# Writes the variable definitions and #defines to the file
# h_file is an open file descriptor to write to
# short_name is the value returned by initialize_names
# variables is a list of variables to include
#
def write_variable_definitions(h_file, short_name, variables):
	sn_upper = short_name.upper()

	# the table comment section
	header_str = make_definition_table_header(sn_upper+" VARIABLE DEFINITIONS %-57.57s"%(" "))
	h_file.write(header_str)
	count = 0 

	for i in variables:
		try: 
			format_table_entry(h_file, i["name"], camputil.convert_mid(nickname, "VAR", count, i), i["description"], i["type"])
		except KeyError, e:
			print "[ Error ] Badly formatted variable. Key not found: "
			print e
			raise
		count += 1 
	h_file.write(" */\n")

	count = 0
	# #defines
	for i in variables:
		try: 
			h_file.write("#define ADM_{0}_VAR_{1}_MID {2}\n".format(sn_upper, i["name"].upper(), camputil.convert_mid(nickname, "VAR", count, i)))
		except KeyError, e:
			print "[ Error ] Badly formatted variable. Key not found: "
			print e
			raise
		count += 1

	h_file.write("\n")

#
# Writes the report template definitions and #defines to the file
# h_file is an open file descriptor to write to
# short_name is the value returned by initialize_names()
# templates is a list of report templates to include
#
def write_rpt_definitions(h_file, short_name, templates):
	sn_upper = short_name.upper()

	# the table comment section
	header_str = make_definition_table_header(sn_upper+" REPORT DEFINITIONS %-58.58s"%(" "))
	h_file.write(header_str)

	count = 0
	for i in templates:
		try: 
			format_table_entry(h_file, i["name"], camputil.convert_mid(nickname, "RPT", count, i), i["description"], "?")
		except KeyError, e:
			print "[ Error ] Badly formatted report-template. Key not found: "
			print e
			raise
		count += 1
		
	h_file.write(" */\n")

	count = 0
	# #defines
	for i in templates:
		try:
			h_file.write("#define ADM_{0}_RPT_{1}_MID {2}\n".format(sn_upper, i["name"].upper(), camputil.convert_mid(nickname, "RPT", count, i)))
		except KeyError, e:
			print "[ Error ] Badly formatted report-template. Key not found: "
			print e
			raise
		count += 1

	h_file.write("\n")

#
# Writes the control definitions and #defines to the file
# h_file is an open file descriptor to write to
# short_name is the value returned by initialize_names()
# controls is a list of controls to include
#
def write_ctrl_definitions(h_file, short_name, controls):
	sn_upper = short_name.upper()

	# the table comment section
	header_str = make_definition_table_header(sn_upper+" CONTROL DEFINITIONS %-56.56s"%(" "))
	h_file.write(header_str)
	
	count = 0
	for i in controls:
		try: 
			format_table_entry(h_file, i["name"], camputil.convert_mid(nickname, "CTRL", count, i), i["description"], "")
		except KeyError, e:
			print "[ Error ] Badly formatted control. Key not found: "
			print e
			raise
		count += 1

	h_file.write(" */\n")

	count = 0
	# #defines
	for i in controls:
		try: 
			h_file.write("#define ADM_{0}_CTRL_{1}_MID {2}\n".format(sn_upper,i["name"].upper(), camputil.convert_mid(nickname, "CTRL", count, i)))
		except KeyError, e:
			print "[ Error ] Badly formatted control. Key not found: "
			print e
			raise
		count += 1
		
	h_file.write("\n")

#
# Writes the constants definitions and #defines to the file
# h_file is an open file descriptor to write to
# short_name is the value returned by initialize_names()
# constants is a list of constants to include
#
def write_const_definitions(h_file, short_name, constants):
	sn_upper = short_name.upper()

	# the table comment section
	header_str = make_definition_table_header(sn_upper+" CONSTANT DEFINITIONS %-56.56s"%(" "))
	h_file.write(header_str)

	count = 4
	for i in constants[4:]:#  first 4 constants are metadata
		try: 
			format_table_entry(h_file, i["name"], camputil.convert_mid(nickname, "CONST", count, i), i["description"], i["type"])
		except KeyError, e:
			print "[ Error ] Badly formatted constant. Key not found: "
			print e
			raise
		count += 1
			
	h_file.write(" */\n")

	count = 4	
	# #defines
	for i in constants[4:]:
		try:
			h_file.write("#define ADM_{0}_CONST_{1}_MID {2}\n".format(sn_upper, i["name"].upper(), camputil.convert_mid(nickname, "CONST", count, i)))
		except KeyError, e:
			print "[ Error ] Badly formatted constant. Key not found: "
			print e
			raise
		count += 1

	h_file.write("\n")

#
# Writes the operator definitions and #defines to the file
# h_file is an open file descriptor to write to
# short_name is the value returned by initialize_names()
# operators is a list of operators to include
#
def write_op_definitions(h_file, short_name, operators):
	sn_upper = short_name.upper()

	# the table comment section
	header_str = make_definition_table_header(sn_upper+" OPERATOR DEFINITIONS %-57.57s"%(" "))
	h_file.write(header_str)

	count = 0
	for i in operators:
		try: 
			format_table_entry(h_file, i["name"], camputil.convert_mid(nickname, "OP", count, i), i["description"], i["result-type"])
		except KeyError, e:
			print "[ Error ] Badly formatted operator. Key not found: "
			print e
			raise
		count += 1
		
	h_file.write(" */\n")

	count = 0
	# #defines
	for i in operators:
		try: 
			h_file.write("#define ADM_{0}_OP_{1}_MID {2}\n".format(sn_upper, i["name"].upper(), camputil.convert_mid(nickname, "OP", count, i)))
		except KeyError, e:
			print "[ Error ] Badly formatted operator. Key not found: "
			print e
			raise
		count += 1

	h_file.write("\n")
	
#
# Writes the macro definitions and #defines to the file
# h_file is an open file descriptor to write to
# short_name is the value returned by initialize_names()
# macros is a list of macros to include
#
def write_macro_definitions(h_file, short_name, macros):
	sn_upper = short_name.upper()

	# the table comment section
	header_str = make_definition_table_header(sn_upper+" MACRO DEFINITIONS %-59.59s"%(" "))
	h_file.write(header_str)

	count = 0
	for i in macros:
		try: 
			format_table_entry(h_file, i["name"], camputil.convert_mid(nickname, "MACRO", count, i), i["description"], "mc")
		except KeyError, e:
			print "[ Error ] Badly formatted macro. Key not found: "
			print e
			raise
		count += 1

	h_file.write(" */\n")

	count = 0
	# #defines
	for i in macros:
		try:
			h_file.write("#define ADM_{0}_MACRO_{1}_MID {2}\n".format(sn_upper, i["name"].upper(), camputil.convert_mid(nickname, "MACRO", count, i)))
		except KeyError, e:
			print "[ Error ] Badly formatted macro. Key not found: "
			print e
			raise
		count += 1

	h_file.write("\n")

#
# Writes the initialization functions' forward declars to the file
# h_file is an open file descriptor to write to
# name is the name returned by initialize_names()
#
def write_initialization_functions(h_file, name):
	init_funct_str = (
		"/* Initialization functions. */\n"
		"void {0}_init();\n"
		"void {0}_init_edd();\n"
		"void {0}_init_variables();\n"
		"void {0}_init_controls();\n"
		"void {0}_init_constants();\n"
		"void {0}_init_macros();\n"
		"void {0}_init_metadata();\n"
		"void {0}_init_ops();\n"
		"void {0}_init_reports();\n"
	)
	h_file.write(init_funct_str.format(name))

#
# Writes the end ifs for at the end of the file
# h_file is an open file desciptor to write to
# name and shortname are the names returned by initialize_names()
#
def write_endifs(h_file, name, short_name):
	endifs_str = (
		"#endif /* _HAVE_{0}_ADM_ */\n"
		"#endif //{1}_H_")
	h_file.write(endifs_str.format(short_name.upper(), name.upper()))
	
#
# Main function of this file, which calls helper funtions to
# orchestrate the creation of the generated file
#
# data: the dictionary from the parsed JSON file
# outpath: the output directory
#
def create(data, outpath, nn):
	global nickname
	nickname = nn

	if nn < 0:
		print "[ Error ] Cannot create ", filename, ": ADM not included in NAME REGISTRY"
		return 
	
	try:
		metadata = data["adm:constants"]
		name, shortname, colon_delim_name, filename = initialize_names(metadata, outpath)
	except KeyError, e:
		print "[ Error ] JSON does not include valid metadata"
		print e
		return


	try:
		h_file = open(filename,"w")
	except IOError, e:
		print "[ Error ] Failed to open ", filename, " for writing."
		print e
		return

	print "Working on ", filename, 
	
	campch.write_standard_h_file_header(h_file, filename)

	write_defines(h_file, name, shortname)	
	write_includes(h_file, shortname)

	write_adm_template_documentation(h_file, colon_delim_name)

	
	write_agent_nickname_definitions(h_file, shortname, nn)

	try:
		write_metadata_definitions(h_file, name, shortname, data.get("adm:constants", []))# metadata is the first 4 constants
		write_edd_definitions(h_file, shortname, data.get("adm:externally-defined-data", []))
		
		write_variable_definitions(h_file, shortname, data.get("adm:variables", []))
		write_rpt_definitions(h_file, shortname, data.get("adm:report-templates", []))
		write_ctrl_definitions(h_file, shortname, data.get("adm:controls", []))
	
		write_const_definitions(h_file, shortname, data.get("adm:constants", []))
		write_macro_definitions(h_file, shortname, data.get("adm:macros", []))
		write_op_definitions(h_file, shortname, data.get("adm:operators", []))
	except KeyError, e:
		return
	finally:
		write_initialization_functions(h_file, name)
		write_endifs(h_file, name, shortname)
		h_file.close()

	print "\t[ DONE ]"	
	


