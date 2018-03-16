# JHU/APL
# Description: creates the c file for the public version of the adm
# Modification History:
#   YYYY-MM-DD     AUTHOR            DESCRIPTION
#   ----------     ---------------   -------------------------------
#   2017-08-11		Evana
#	2017-12-28     David 			Metadata is in constants now
###############################################

import re
import os
import errno

import camputil
import campch

#
# Constructs the name, shortname, and filename for this generated file.
# Returns a tuple in that order
#
# Also creates the mgr directory in the out directory if
# it doesn't exist
#
def initialize_names(metadata, outpath):
	short_name = re.sub('\s','_',metadata[0]["value"]).lower()
	name       = "adm_" + short_name

	# Make the mgr dir, only allow the 'directory exists' error
	try:
		os.mkdir(outpath+"/mgr/")
	except OSError as ose:
		if(not ose.errno == errno.EEXIST):
			print "[ Error ] Failed to create subdirectory in ", outpath
			print ose
			exit(-1)
			
	filename = outpath + "/mgr/" + name + "_mgr.c"
	return name, short_name, filename

#
# Returns a string for a valid call to the names_add_name() function
# name is the name returned from a call to initialize_names()
# ttype is the type of item (EDD, VAR, etc)
# item is the items to add. Assumes item contains a value for "name" and
# "description"
#
def make_names_add_name_string(name, ttype, item):
	add_name_str = "\tnames_add_name(\"{0}\", {1}, {2}, {3});\n"
	
	mid = campch.make_mid_string(name, ttype, item)

	return add_name_str.format(item["name"].upper(),
				   campch.parameter_format(item["description"]),
				   name.upper(), mid)

#
# Writes the #includes to the open file descriptor passed, c_file
# short_name is the short_name value returned by initialize_names()
#
def write_includes(c_file, short_name):
	files = ["ion.h", "lyst.h", "platform.h",
		 "../shared/adm/adm_{}.h".format(short_name),
		 "../shared/utils/utils.h",   "../shared/primitives/def.h",
		 "../shared/primitives/nn.h", "../shared/primitives/report.h",
		 "../shared/primitives/blob.h", 
		 "nm_mgr_names.h", "nm_mgr_ui.h"]

	c_file.write(campch.make_include_str(files))

#
# Write the #defines to the open file descriptor passed, c_file
# short name is the short name value returned by initialize_names()
#
def write_defines(c_file, short_name):
	sn_upper = short_name.upper()
	define_str = (
		"\n#define _HAVE_{0}_ADM_"
		"\n#ifdef _HAVE_{0}_ADM_"
		"\n")
	c_file.write(define_str.format(sn_upper))

#
# Writes the top-level init function that calls all of the other ones
# c_file is an open file descriptor to write to
# name is the name returned from initialize_names()
#
def write_init_function(c_file, name):
	body = (
		"\n\t{0}_init_edd();"
		"\n\t{0}_init_variables();"
		"\n\t{0}_init_controls();"
		"\n\t{0}_init_constants();"
		"\n\t{0}_init_macros();"
		"\n\t{0}_init_metadata();"
		"\n\t{0}_init_ops();"
		"\n\t{0}_init_reports();"
		"\n\n")
	campch.write_formatted_init_function(c_file, name, None, body.format(name))

#
# Writes the init_edds function to the passed open file c_file
# name and short_name are the values returned by initialize_names()
# edds is a list of edds to include
#
def write_init_edd_function(c_file, name, short_name, edds):
	body = ""
	ttype = "edd"
	
	for i in edds:
		try:
			mid = campch.make_mid_string(name, ttype, i)
			amp_type = campch.make_amp_type(i)
			body = body + campch.make_adm_add_edd_string(mid, amp_type, "0", "NULL",
								     "NULL", "NULL")
			
			body = body + make_names_add_name_string(name, ttype, i)
			body = body + campch.make_add_parmspec_string(name, ttype, i)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise
		
	campch.write_formatted_init_function(c_file, name, ttype, body)

#
# Writes the init_variables() function to the open file descriptor passed as c_file
# name is the value returned from initialize_names()
# variables is a list of variables to include
#
def write_init_variables_function(c_file, name, variables):
	body = ""
	ttype = "var"

	for i in variables:
		try:
			amp_type = campch.make_amp_type(i)

			body = body + campch.make_adm_add_var_string(name, i, amp_type, "NULL")			
			body = body + make_names_add_name_string(name, ttype, i)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, "variables", body)

#
# Writes the init_controls() function to the open file descriptor passed as c_file
# name is the value returned from initialize_names()
# controls is a list of controls to include
#
def write_init_controls_function(c_file, name, controls):
	body = ""
	ttype = "ctrl"
	
	for i in controls:
		try: 
			mid = campch.make_mid_string(name, ttype, i)
			body = body + campch.make_adm_add_ctrl_string(name, i, "NULL")
			body = body + make_names_add_name_string(name, ttype, i)
			body = body + campch.make_add_parmspec_string(name, ttype, i)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, "controls", body)

#
# Writes the init_constants() function to the open file descriptor passed as c_file
# name is the value returned from initialize_names()
# constants is a list of constants to include
#
def write_init_constants(c_file, name, constants):
	body = ""
	ttype = "const"

	for i in constants[4:]: #first 4 constants are metadata
		try: 
			mid = campch.make_mid_string(name, ttype, i)
			body = body + campch.make_adm_add_const_string(name, i, "NULL")
			body = body + make_names_add_name_string(name, ttype, i)
			body = body + campch.make_add_parmspec_string(name, ttype, i)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise
		
	campch.write_formatted_init_function(c_file, name, "constants", body)

#
# Writes the init_macros() function to the open file descriptor passed as c_file
# name is the value returned from initialize_names()
# macros is a list of macros to include
#
def write_init_macros(c_file, name, macros):
	body = ""
	ttype = "macro"
	plural_ttype = ttype + "s"
	
	for i in macros:
		try: 
			mid = campch.make_mid_string(name, ttype, i)
			body = body + campch.make_adm_add_macro_string(name, i, "NULL")
			body = body + make_names_add_name_string(name, ttype, i)
			body = body + campch.make_add_parmspec_string(name, plural_ttype, i)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, plural_ttype, body)

#
# Writes the init_metadata() function to the open file descriptor passed as c_file
# name and short_name are the values returned from initialize_names()
# metadata is a list of the metadata to include
#
def write_init_metadata(c_file, name, short_name, metadata):
	
	body = "\n\t/* Step 1: Register Nicknames */\n"
	for ttype in ["META", "EDD", "VAR", "RPT", "CTRL", "CONST", "MACRO", "OP", "ROOT"]:
		body = body + campch.make_oid_nn_add_parm_str(short_name, ttype)

	body = body + "\n\t/* Step 2: Register Metadata Information. */\n"
	for i in metadata[:4]: # first 4 of constants is metadata
		try:
			mid = campch.make_mid_string(name, "META", i)
			amp_type = campch.make_amp_type(i)
			body = body + campch.make_adm_add_edd_string(mid, amp_type, "0", "NULL",
								     "adm_print_string", "adm_size_string")
			body = body + make_names_add_name_string(name, "META", i)
		except KeyError, e:
			print "[ Error ] Badly formatted metadata. Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, "metadata", body)

#
# Writes the init_operators() function to the open file descriptor passed as c_file
# name is the value returned from initialize_names()
# operators is a list of the operators to include
#
def write_init_ops(c_file, name, operators):
	body = ""
	ttype = "op"
	plural_ttype = ttype + "s"
	
	for i in operators:
		try :
			mid = campch.make_mid_string(name, ttype, i)

			body = body + campch.make_adm_add_ops_string(name, i, "NULL")
			body = body + make_names_add_name_string(name, ttype, i)
			body = body + campch.make_add_parmspec_string(name, ttype, i)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, plural_ttype, body)

#
# Writes the init_reports() function to the open file descriptor passed as c_file
# name is the value returned from initialize_names()
# reports is a list of reports to include
#
def write_init_reports(c_file, name, data):
	reports = data.get("adm:report-templates", [])

	name_upper = name.upper()
	
	body = ("\n\t"
		"Lyst rpt                = NULL;\n\t"
		"mid_t *cur_mid          = NULL;\n\t"
		"rpttpl_item_t *cur_item = NULL;\n\t"
		"uint32_t used           = 0;\n\t"
		"\n\t")

	lyst_create_str  = "rpt = lyst_create();\n\n\t"
	cur_mid_template = "cur_mid = mid_from_value({0}_{1}_{2}_MID);\n\t"
	lyst_insert_str  = "lyst_insert_last(rpt,cur_item);\n\n\t"
	destroy_str      = "midcol_destroy(&rpt);\n\n"

	for i in reports:
		body = body + lyst_create_str

		params = i.get("parmspec", [])

		try:
			for d in i["definition"]:
				d_type, d_name, d_params = campch.parse_definition(d)

				body = body + cur_mid_template.format(name_upper, d_type, d_name.upper())

				body = body + campch.make_rpt_parm_calls_string(d, params, data)

				body = body + lyst_insert_str

			body = body + destroy_str.format(name_upper, i["name"].upper())
			body = body + make_names_add_name_string(name, "RPT", i)
		except KeyError, e:
			print "[ Error ] Badly formatted report. Key not found:"
			print e
			raise
		except Exception:
			print "[ Error ] Encountered problem formatting parameterized reports"
			raise

	campch.write_formatted_init_function(c_file, name, "reports", body)
	
#
# Main function of this file, which calls helper functions to
# orchestrate the creation of the generated file
#
# data: the dictionary made from a parsed JSON file
# outpath: the output directory
#
def create(data, outpath):
	try:
		metadata = data["adm:constants"]
		name, short_name, filename = initialize_names(metadata, outpath)
	except KeyError, e:
		print "[ Error ] JSON does not include valid metadata"
		print e
		return

	# Create the new file
	try:
		c_file = open(filename,"w")
	except IOError, e:
		print "[ Error ] Failed to open ", filename, " for writing."
		print e
		return

	print "Working on ", filename,
	
	campch.write_standard_c_file_header(c_file, filename)

	# calling each of the helper functions to handle the writing of
	# various functions in this file
	
	write_includes(c_file, short_name)
	write_defines(c_file, short_name)

	write_init_function(c_file, name);

	try: 
		write_init_edd_function(c_file, name, short_name, data.get("adm:externally-defined-data", []))
		write_init_variables_function(c_file, name, data.get("adm:variables", []))
		write_init_controls_function(c_file, name, data.get("adm:controls", []))
		write_init_constants(c_file, name, data.get("adm:constants", []))
		write_init_macros(c_file, name, data.get("adm:macros", []))

		write_init_metadata(c_file, name, short_name, data.get("adm:constants", []))
		
		write_init_ops(c_file, name, data.get("adm:operators", []))
		
		write_init_reports(c_file, name, data)
	except KeyError, e:
		return
	except Exception:
		return
	finally:
		c_file.write("#endif // _HAVE_"+short_name.upper()+"_ADM_\n")
		c_file.close()
	
	print "\t[ DONE ]"
