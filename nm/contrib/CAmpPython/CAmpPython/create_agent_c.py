# JHU/APL
# Description: creates the c file for the public version of the adm
# Modification History:
#   YYYY-MM-DD     AUTHOR            DESCRIPTION
#   ----------     ---------------   -------------------------------
#   2017-08-11		Evana
#	2017-12-28     David 			Metadata is in constants now
#
###############################################

import re
import os
import errno

import campch

#
# Constructs the name, shortname, and filename for
# this generated file. Returns as a tuple in that order.
#
# Also creates the agent directory in the out directory
# if it doesn't exist.
#
# metadata is the metadata entry in the dictionary built from the JSON file
# outpath is the path to the output directory
#
def initialize_names(metadata, outpath):
	short_name = re.sub('\s', '_', metadata[0]["value"]).lower()
	name       = "adm_" + short_name

	# Make the agent dir, only allow the 'directory exists' error
	try:
		os.mkdir(outpath + "/agent/")
	except OSError as ose:
		if(not ose.errno == errno.EEXIST):
			print "[ Error ] Failed to create subdirectory in ", outpath
			print ose
			exit(-1)
			
	filename = outpath + "/agent/" + name + "_agent.c"		
	return name, short_name, filename

# 
# helper function for writing the function for 
# variable definitions 
# creates and writes the entries for the expression lyst to 
# c_file 
#
def make_parm_lyst_string(name, var):
	out = "\tdef = lyst_create();\n"

	lyst_insert_str = "\tlyst_insert_last(def,mid_from_value({0}_{1}_{2}_MID));\n"
		
	try:
		for i in var["initializer"]["postfix-expr"]:
			type, item_name = i.split(".")
			out = out + lyst_insert_str.format(name.upper(), type.upper(), item_name.upper())
	except KeyError, e:
		print "[Error ] Badly formatted variable. Key not found:"
		print e
		raise
	
	return out + "\n"

#
# Writes all of the #includes for this c file
#
# c_file is an open file descriptor to be written to
# name is the name provided by initialize_names()
#
def write_includes(c_file, name):
	files = ["ion.h", "lyst.h", "platform.h",
		 "../shared/adm/{}.h".format(name), "../shared/utils/utils.h",
		 "../shared/primitives/def.h",      "../shared/primitives/nn.h", 
		 "../shared/primitives/report.h",   "../shared/primitives/blob.h",
		 "{}_impl.h".format(name),          "rda.h"]

	c_file.write(campch.make_include_str(files))

#
# Writes the init function to the file
# Includes a custom body tag with TODO
#
# c_file is an open file descriptor to write to
# name is the basic name provided by initialize_names()
#
def write_init_function(c_file, name):
	body = (
		"\n\t{0}_setup();"
		"\n\t{0}_init_edd();"
		"\n\t{0}_init_variables();"
		"\n\t{0}_init_controls();"
		"\n\t{0}_init_constants();"
		"\n\t{0}_init_macros();"
		"\n\t{0}_init_metadata();"
		"\n\t{0}_init_ops();"
		"\n\t{0}_init_reports();"
		"\n")
	campch.write_formatted_init_function(c_file, name, None, body.format(name))
	
#
# Constructs and writes the init_edd function
#
# c_file is an open file descriptor to write to
# name is the value returned from initialize_names()
# edds is a list of the edds to add
#
def write_init_edd_function(c_file, name, edds):
	body = ""
	ttype = "edd"
	
	for i in edds:
		try:
			mid = campch.make_mid_string(name, ttype, i)
			amp_type = campch.make_amp_type(i)
			collect = "{0}_get_{1}".format(name, i["name"])
							
			body = body + campch.make_adm_add_edd_string(mid, amp_type, "0", collect,
								     "NULL", "NULL")
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, ttype, body);

#
# Constructs and writes the init_controls function
#
# c_file is an open file descriptor to write to,
# name is the value returned from initialize_names()
# controls is a list of controls to add
#
def write_init_control_function(c_file, name, controls):
	body = ""
	ttype = "ctrl"

	for i in controls:
		try:
			add_ctrl_parameter = "{0}_{1}_{2}".format(name, ttype, i["name"])
			
			body = body + campch.make_adm_add_ctrl_string(name, i, add_ctrl_parameter)

		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, "controls", body)

#
# Constructs and writes the init_constants function
#
# c_file is an open file descriptor to write to,
# name is the value returned from initialize_names()
# constants is a list of constants to add
#
def write_init_constant_function(c_file, name, constants):
	body = ""
	ttype = "const"
	
	for i in constants[4:]: # first 4 constants are metadata
		try:
			add_const_parameter = "{0}_{1}_{2}".format(name, ttype, i["name"])
			body = body + campch.make_adm_add_const_string(name, i, add_const_parameter)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, "constants", body)

#
# Constructs and writes the init_macros function
#
# c_file is an open file descriptor to write to,
# name is the value returned from initialize_names()
# macros is a list of macros to add
#
def write_init_macro_function(c_file, name, macros):
	body = "" 
	ttype = "macro"
	plural_ttype = ttype+"s"
	
	for i in macros:
		try:
			add_macro_parameter = "{0}_{1}_{2}".format(name, ttype, i["name"])
			body = body + campch.make_adm_add_macro_string(name, i, add_macro_parameter)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise
		
	campch.write_formatted_init_function(c_file, name, plural_ttype, body)

#
# Constructs and writes the init_operators function
#
# c_file is an open file descriptor to write to,
# name is the value returned from initialize_names()
# operators is a list of operatorss to add
#
def write_init_op_function(c_file, name, operators):
	body = ""
	ttype = "op"
	plural_ttype = ttype+"s"

	for i in operators:
		try:
			add_ops_parameter = "{0}_{1}_{2}".format(name, plural_ttype, i["name"])
			body = body + campch.make_adm_add_ops_string(name, i, add_ops_parameter)
		except KeyError, e:
			print "[ Error ] Badly formatted ",ttype,". Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, plural_ttype, body)

#
# Writes the init_variables body 
# for each variable creates a lyst for its postfix-expr and writes 
# its appropriate adm_add_var function
#
def write_init_var_function(c_file, name, variables):
	body = ("\n\t"
		"uint32_t used  = 0;\n\t"
		"mid_t *cur_mid = NULL;\n\t"
		"expr_t *expr   = NULL;\n\t"
		"Lyst def       = NULL;\n"
		"\n")
	create_template = "\texpr = expr_create(AMP_TYPE_{0}, def);\n\n"
	release_str     = "\texpr_release(expr);\n\n"

	for i in variables:
		try:
			body = body + make_parm_lyst_string(name, i)
			body = body + create_template.format(i["type"])

			add_var_parameter = "{0}_{1}_{2}".format(name, "var", i["name"])

			amp_type = campch.make_amp_type(i)
			body = body + campch.make_adm_add_var_string(name, i, amp_type, "expr")

			body = body + release_str

		except KeyError, e:
			print "[ Error ] Badly formatted variable. Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, "variables", body)

#
# Constructs and writes the init metadata function
#
# c_file is an open file descriptor to write to
# name is the name returned by a call to initialize_names()
# metadata is a list of metadata to add
#
def write_init_metadata_function(c_file, name, short_name, metadata):
	body = "\n\t/* Step 1: Register Nicknames */\n"

	for ttype in ["META", "EDD", "VAR", "RPT", "CTRL", "CONST", "MACRO", "OP", "ROOT"]:
		body = body + campch.make_oid_nn_add_parm_str(short_name, ttype)
		
	body = body + "\n\t/* Step 2: Register Metadata Information. */\n"
	for i in metadata[:4]: # first 4 of constants is metadata
		try:
			mid = campch.make_mid_string(name, "META", i)
			amp_type = campch.make_amp_type(i)
			collect = "{0}_meta_{1}".format(name, i["name"])

			body = body + campch.make_adm_add_edd_string(mid, amp_type, "0", collect,
								     "adm_print_string", "adm_size_string")
		except KeyError, e:
			print "[ Error ] Badly formatted metadata. Key not found:"
			print e
			raise

	campch.write_formatted_init_function(c_file, name, "metadata", body)
	
#
# Constructs and writes the init reports function
#
# c_file is an open file descriptor to write to
# name is the name returned by the call to initialize_names()
# data is a dictionary of the JSON file. This function needs
# the entire dictionary in order to determine the type of parameter in a
# statcially parameterized report
#
def write_init_reports_function(c_file, name, data):
	templates = data.get("adm:report-templates", [])

	name_upper = name.upper()

	body = ("\n\t"
		"Lyst rpt                = NULL;\n\t"
		"mid_t *cur_mid          = NULL;\n\t"
		"rpttpl_item_t *cur_item = NULL;\n\t"
		"uint32_t used           = 0;\n\t"
		"\n\t")

	lyst_create_str  = "rpt = lyst_create();\n\n\t"
	cur_mid_template = "cur_mid = mid_from_value({0}_{1}_{2}_MID);\n\t"
	lyst_insert_str  = "lyst_insert_last(rpt, cur_item);\n\n\t"
	destroy_str      = "midcol_destroy(&rpt);\n\n\t"

	for i in templates:
		body = body + lyst_create_str

		params = i.get("parmspec", [])

		try:
			for d in i["definition"]:
				d_type, d_name, d_params = campch.parse_definition(d)

				body = body + cur_mid_template.format(name_upper, d_type, d_name.upper())
				
				body = body + campch.make_rpt_parm_calls_string(d, params, data)
				
				body = body + lyst_insert_str 

			body = body + destroy_str.format(name_upper, i["name"].upper())

		except KeyError, e:
			print "[ Error ] Badly formatted report-template. Key not found:"
			print e
			raise
		except Exception:
			print "[ Error ] Encountered problem formatting parameterized reports"
			raise

	campch.write_formatted_init_function(c_file, name, "reports", body)
	
#
# Main function of this file, which calls helper functions
# to orchestrate creation of the generated file
#
# data: the dictionary from the parsed JSON file
# outpath: the output directory
#
def create(data, outpath):
	try:
		metadata = data["adm:constants"]
		name, shortname, filename = initialize_names(metadata, outpath)
		sn_upper = shortname.upper()
	except KeyError, e:
		print "[ Error ] JSON does not include valid metadata"
		print e
		return
	
	try:
		c_file = open(filename,"w")
	except IOError, e:
		print "[ Error ] Failed to open ", filename, " for writing."
		print e
		return

	print "Working on ", filename, 

	# Standard header comment, includes and #defines
	campch.write_standard_c_file_header(c_file, filename)
	write_includes(c_file, name)

	c_file.write("#define _HAVE_"+sn_upper+"_ADM_\n")
	c_file.write("#ifdef _HAVE_"+sn_upper+"_ADM_\n\n")

	# Init function
	write_init_function(c_file, name)

	try: 
		write_init_edd_function(c_file, name, data.get("adm:externally-defined-data", []))

		write_init_var_function(c_file,      name, data.get("adm:variables", []))
		write_init_control_function(c_file,  name, data.get("adm:controls", []))
		write_init_constant_function(c_file, name, data.get("adm:constants", []))
		write_init_macro_function(c_file,    name, data.get("adm:macros", []))
		write_init_op_function(c_file,       name, data.get("adm:operators", []))

		write_init_metadata_function(c_file, name, shortname, data.get("adm:constants", [])[:4])

		# Init reports function
		write_init_reports_function(c_file, name, data) 

	except KeyError, e:
		return
	except Exception:
		return
	finally:
		c_file.write("#endif // _HAVE_"+sn_upper+"_ADM_\n")
		c_file.close()
	
	print "\t[ DONE ]"
