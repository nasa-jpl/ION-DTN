# JHU/APL
# Description: library file for common functions related to the generation of
# .c and .h files
# Modification History:
#   YYYY-MM-DD     AUTHOR            DESCRIPTION
#   ----------     ---------------   -------------------------------
#   2017-11-21     Sarah             File creation
#
##############################################################################
#
# TODO: this file needs cleaned up, organized. Maybe split into multiple utils

import os
import re
import datetime

#
# Writes the standard .c file header to the file
#
# file: open file descriptor to write to
# filepath: the path to the of the file, where the basename will be included
# as part of the header
#
def write_standard_c_file_header(file, filepath):
	standard_c_header = (
		"/****************************************************************************\n"
		" **\n"
		" ** File Name: {}\n"
		" **\n"
		" ** Description: TODO\n"
		" **\n"
		" ** Notes: TODO\n"
		" **\n"
		" ** Assumptions: TODO\n"
		" **\n"
		" ** Modification History: \n"
		" **  YYYY-MM-DD  AUTHOR           DESCRIPTION\n"
		" **  ----------  --------------   --------------------------------------------\n"
		" **  {}  AUTO             Auto-generated c file \n"
		" **\n"
		" ****************************************************************************/\n\n")

	file.write(standard_c_header.format(os.path.basename(filepath), str(datetime.datetime.now().date())))

#
# Writes the standard .h file header to the file
#
# file: open file descriptor to write to
# filepath: the path to the of the file, where the basename will be included
# as part of the header
#
def write_standard_h_file_header(file, filepath):
	standard_h_header = (
		"/****************************************************************************\n"
		" **\n"
		" ** File Name: {}\n"
		" **\n"
		" ** Description: TODO\n"
		" **\n"
		" ** Notes: TODO\n"
		" **\n"
		" ** Assumptions: TODO\n"
		" **\n"
		" ** Modification History: \n"
		" **  YYYY-MM-DD  AUTHOR           DESCRIPTION\n"
		" **  ----------  --------------   --------------------------------------------\n"
		" **  {}  AUTO             Auto-generated header file \n"
		" **\n"
		" ****************************************************************************/\n\n")

	file.write(standard_h_header.format(os.path.basename(filepath), str(datetime.datetime.now().date())))

#
# Write the standard 'CUSTOM' tag for the includes at the top of a file
# Adds any passed custom content between the start and stop tags
#
# file: open file descriptor to write to
# custom: array of lines to add as custom content (from scraping)
# if custom is empty, will just write the tags to the file
#
def write_custom_includes(file, custom):
	file.write("/*   START CUSTOM INCLUDES HERE  */\n")
	for line in custom:
		file.write(line);
	file.write("/*   STOP CUSTOM INCLUDES HERE  */\n\n")

#
# Pops items off of the passed queue (list) structure, searching
# for the custom includes tags. Returns all lines encompassed in these tags
#
# lines: lines to search
# Returns: (list, list) tuple that is 1) list of strings from between the custom
# includes tags and 2) updated 'lines' queue (evaluated lines are popped off)
#
# NOTICE: since this is treating lines as a queue, it will evaluate lines in
# reverse order (popping off the end of the list).
#
def find_custom_includes_in_queue(lines):
	includes = []
	line = ""
	
	# find the start
	while (len(lines) != 0 and re.match('\/\*   START CUSTOM INCLUDES HERE  \*\/',line.strip()) == None):
		line = lines.pop()

	# Append until we find the end
	while(len(lines) != 0):
		line = lines.pop()
		clean_line = line.strip();
		if(re.match('\/\*   STOP CUSTOM INCLUDES HERE  \*\/',clean_line) != None):
			break
		includes.append(line)
		
	return includes, lines
	
#
# Write the standard 'CUSTOM' tag for custom functions
# Adds any passed custom content between the start and stop tags
#
# file: open file descriptor to write to
# custom: array of lines to add as custom content (from scraping)
# if custom is empty, will just write the tags for custom content to the file
#
def write_custom_functions(file, custom):
	file.write("/*   START CUSTOM FUNCTIONS HERE */\n")
	for line in custom:
		file.write(line)
	file.write("/*   STOP CUSTOM FUNCTIONS HERE  */\n\n")

#
# Pops items off of the passed queue (list) structure, searching
# for the custom functions tags. Returns all lines encompassed in these tags
#
# lines: lines to search
# Returns: (list, list) tuple that is 1) list of strings from between the custom
# function tags and 2) updated 'lines' queue (evaluated lines are popped off)
#
# NOTICE: since this is treating lines as a queue, it will evaluate lines in
# reverse order (popping off the end of the list).
#
def find_custom_func_in_queue(lines):
	custom_func = []
	line = ""
	
	# find the start
	while (len(lines) != 0 and re.match('\/\*   START CUSTOM FUNCTIONS HERE \*\/',line.strip()) == None):
		line = lines.pop()

	# Append until we find the end
	while(len(lines) != 0):
		line = lines.pop()
		clean_line = line.strip()
		if(re.match('\/\*   STOP CUSTOM FUNCTIONS HERE  \*\/',clean_line) != None):
			break
		custom_func.append(line)
		
	return custom_func, lines
		
#
# Write the standard 'CUSTOM' tag for the body of a standard function
#
# file: open file descriptor to write to
# function: function name
# custom: array of lines to add as custom function body content (from scraping)
# if custom is empty, will just write the tags for custom content to the file
#
def write_function_custom_body(file, function, custom):
	custom_function_body_tag = (
		"\t/*\n"
		"\t * +-------------------------------------------------------------------------+\n"
		"\t * |{} CUSTOM FUNCTION {} BODY\n" 
		"\t * +-------------------------------------------------------------------------+\n"
		"\t */\n")
	
	file.write(custom_function_body_tag.format("START", function))
	
	for line in custom:
		file.write(line)

	file.write(custom_function_body_tag.format("STOP", function))
	
#
# Pops items off of the passed queue (list) structure, searching
# for the function custom body tags. Returns all dictonary of key:value pairs for
# lines encompassed in these tags, where the key is the function name, and value
# is the list of custom lines for that function
# This exhausts the entire passed queue, so unlike the other find*_custom_*() functions,
# it does not return a list of remaining lines in the queue
#
# lines: lines to search
#
# NOTICE: since this is treating lines as a queue, it will evaluate lines in
# reverse order (popping off the end of the list).
#
def find_func_custom_body_in_queue(lines):
	func_bods = {}
	func = None

	# While lines remain in the queue, pop one off and evaluate it
	while len(lines) != 0:
		line = lines.pop()
		clean_line = line.strip()

		# If we're inside one of the custom function bodies
		if(func is not None):
			# If it matches the other part of the CUSTOM tag comment, check if need to skip or stop
			if(re.match('(\* \+-------------------------------------------------------------------------\+)',clean_line) != None):
				line = lines.pop()
				clean_line = line.strip()
	
				# If it matches the end tag, stop
				if(re.match('\* \|STOP CUSTOM FUNCTION (.+) BODY', clean_line) != None):
					func_bods[func].pop()
					func = None
				else:		
					continue
			
			# Otherwise, append to that function's dictionary entry
			else:
				if(not func in func_bods):
					func_bods[func] = []
				func_bods[func].append(line)
				
		# If we're not inside a custom function body, check if this is the start of one
		else:
			s = re.search('\* \|START CUSTOM FUNCTION (.+) BODY', clean_line)
			if s != None:
				func = s.group(1)

	return func_bods

#
# writes the type enum tags and if scraping was required any 
# typeENUMS that were found
def write_type_enums(file, custom):
	file.write("/*   START typeENUM */\n")
	for line in custom:
		file.write(line)
	file.write("/*   STOP typeENUM  */\n\n")


#
# Pops items off of the passed queue (list) structure, searching
# for the type_enum tag. Returns all lines encompassed in these tags
#
# lines: lines to search
# Returns: (list, list) tuple that is 1) list of strings from between the custom
# function tags and 2) updated 'lines' queue (evaluated lines are popped off)
#
# NOTICE: since this is treating lines as a queue, it will evaluate lines in
# reverse order (popping off the end of the list).
#
def find_type_enums_in_queue(lines):
	enums = []
	line = ""
	
	# find the start
	while (len(lines) != 0 and re.match('\/\*   START typeENUM \*\/',line.strip()) == None):
		line = lines.pop()

	# Append until we find the end
	while(len(lines) != 0):
		line = lines.pop()
		clean_line = line.strip();
		if(re.match('\/\*   STOP typeENUM  \*\/',clean_line) != None):
			break
		enums.append(line)

	return enums, lines



#
# Scrapes the passed file for CUSTOM tags to be put into freshly-generated
# file. Returns a (list, list, dictionary) tuple of 1) custom includes,
# 2) custom functions, and 3) function custom bodies
#
# In the function custom bodies dictionary, the name of the function serves
# as the key, and the value is a list of strings that make up the custom body
# of that function.
#
# file: c file to scrape
#
def scrape(file):
	c = []
	includes = []
	custom_func = []
	func_bods = {}

	# Insert each line into a queue
	# NOTE: this results in the first line of the file being last in c
	# (find_* functions appropriately pop off the end of c).
	# NOTE: we're keeping whitespace here in order to preserve any of the user's
	# custom code. Remember to account for whitespace when pattern matching.
	try:
		for line in open(file).readlines(): 
			c.insert(0,line)
	except IOError, e:
		print "[ Error ] Failed to open ", file, " for scraping."
		print e
		return includes, custom_func, func_bods

	includes,    c = find_custom_includes_in_queue(c)
	custom_func, c = find_custom_func_in_queue(c)
	func_bods      = find_func_custom_body_in_queue(c)
	
	return includes, custom_func, func_bods


#
# Scrapes the passed file for CUSTOM tags to be put into freshly-generated
# file. Returns a (list, list) tuple of 1) custom includes, and
# 2) custom functions
#
# file: h file to scrape
#
def scrapeH(file):
	h = []
	includes = []
	custom_func = []
	type_enums = []
	

	# Insert each line into a queue
	# NOTE: this results in the first line of the file being last in h
	# (find_* functions appropriately pop off the end of h).
	# NOTE: we're keeping whitespace here in order to preserve any of the user's
	# custom code. Remember to account for whitespace when pattern matching.
	try:
		for line in open(file).readlines(): 
			h.insert(0,line)
	except IOError, e:
		print "[ Error ] Failed to open ", file, " for scraping."
		print e
		return includes, custom_func

	includes,    h = find_custom_includes_in_queue(h)
	type_enums,  h = find_type_enums_in_queue(h)
	custom_func, h = find_custom_func_in_queue(h)
	
	return includes, custom_func, type_enums

#
# Takes the passed in string, and replaces any double quotes (")
# with (\"), any newlines with (\\n), and adds \" to the
# beginning and end of the string in an effort to make it safe
# for passing as a string parameter to a c function.
# tainted is the string to reformat, returns the result
#
def parameter_format(tainted):
	tainted = re.sub('\"', '\\"', tainted)
	tainted = re.sub('\\n', '\\\n', tainted)
	untainted = "\"" + tainted + "\""
	return untainted

#
# Takes the passed string, and adds c comment character to make it
# print well as a multi-line comment in a c file
#
def multiline_comment_format(tainted):
	comment_length = 100
	
	# replace any comment-ending characters with empty string
	tainted = re.sub('\*/', '', tainted)
	
	# replace newline with newline + comment character
	tainted = re.sub('\\n', '\\n *', tainted)

	# split the lines every x characters to wrap
	tainted_list = tainted.splitlines()
	untainted = []
	for line in tainted_list:
		while len(line) > comment_length:
			left, right = line[:comment_length], line[comment_length:]
			untainted.append("\n * " + left)
			line = right
		untainted.append("\n * " + line) 

	# Add opening and closing comment characters
	untainted = ("/*" + "".join(untainted) + "\n */")
	return untainted

#
# Given a list of files, creates the string neccessary to #include all of
# them in a .[ch] file
# Returns the completed string.
#
def make_include_str(files):
	include_str = ""
	for file in files:
		include_str = include_str + "\n#include \"{}\"".format(file)
	include_str = include_str + "\n\n"

	return include_str

#
# Writes an init function to the passed c_file.
# name is the name returned by a call to initialize_names,
# ttype can be None and is the type of init function to create
# body is the body of the function.
#
def write_formatted_init_function(c_file, name, ttype, body):
	if ttype is not None:
		ttype = "_" + ttype
	else:
		ttype = ""
		
	init_funct_string = (
		"void {0}_init{1}()"
		"\n{{"
		"\n{2}"
		"\n}}"
		"\n\n")

	c_file.write(init_funct_string.format(name, ttype, body))

#
# Makes and returns the function call to UI_ADD_PARMSPEC_* for the item passed
# name is the name returned by the call to initialize_names, item is the edd, variable, etc. to call
# UI_ADD_PARMSPEC_ for, ttype is [edd|var|op..]
#
def make_add_parmspec_string(name, ttype, item):
	
	# If there are no parameters, return
	params = item.get("parmspec", [])
	if params == []:
		return "\n"

	call_template  = "\tUI_ADD_PARMSPEC_{0}({1}_{2}_{3}_MID"
	param_template = ", \"{0}\", AMP_TYPE_{1}"
	i_name   = item["name"]

	add_parmspec_str = call_template.format(str(len(params)), name.upper(), ttype.upper(), i_name.upper())

	# Add each parameter to the function call
	for parameter in params:
		for (key, value) in parameter.iteritems():
			add_parmspec_str = add_parmspec_str + param_template.format(value, key)
		
	return add_parmspec_str + ");\n\n"

#
# Formats and returns a string for the mid
# name is the value returned from a call to initialize_names, ttype is the type
# of item to make (VAR, EDD, etc), item is the item to make the mid for
# Assumes that the passed item has a valid "name" field
#
def make_mid_string(name, ttype, item):
	template = "{0}_{1}_{2}_MID"
	return template.format(name.upper(), ttype.upper(), item["name"].upper())

#
# Makes and returns the amp type string for the passed item
# Assumes that the item has a valid "type" field
#
def make_amp_type(item):
	return "AMP_TYPE_{}".format(item["type"].upper())

#
# Generic function for making an adm_add_*() function call
# ttype is the type of adm_add_function to call (edd, var, etc)
# mid is the mid of the item to add, params is a list of strings of
# any additional parameters to pass to the adm_add_*() function
#
def make_adm_add_generic_string(ttype, mid, params):
	template = "\tadm_add_{0}(mid_from_value({1})"
	param_template = ", {}"
	
	out = template.format(ttype, mid)
	
	for p in params:
		out = out + param_template.format(p)
		
	out = out + ");\n"
	return out

#
# Makes and returns the function call string for adm_add_edd
# mid is the mid of this item
# the rest of the passed values are parameters to pass to the adm_add_edd function
#
def make_adm_add_edd_string(mid, amp_type, num_parms, collect, to_string, size):
	params = [amp_type, num_parms, collect, to_string, size]
	return make_adm_add_generic_string("edd", mid, params)

#
# Makes and returns the function call string for adm_add_ctrl
# name is the name returned by the call to initialize_names(), item is the object to add,
# param is the parameter to pass to adm_add_macro
#
def make_adm_add_ctrl_string(name, item, param):
	params = [param]
	mid = make_mid_string(name, "ctrl", item)
	return make_adm_add_generic_string("ctrl", mid, params)

#
# Makes and returns the function call string for adm_add_const
# name is the name returned by the call to initialize_names(), item is the object to add,
# param is the parameter to pass to adm_add_macro
#
def make_adm_add_const_string(name, item, param):
	params = [param]
	mid = make_mid_string(name, "const", item)
	return make_adm_add_generic_string("const", mid, params)

#
# Makes and returns the function call string for adm_add_macro
# name is the name returned by the call to initialize_names(), item is the object to add,
# param is the parameter to pass to adm_add_macro
#
def make_adm_add_macro_string(name, item, param):
	params = [param]
	mid = make_mid_string(name, "macro", item)
	return make_adm_add_generic_string("macro", mid, params)

#
# Makes and returns the function call string for adm_add_ops
# name is the name returned by the call to initialize_names(), item is the object to add,
# param is the parameter to pass to adm_add_ops
#
def make_adm_add_ops_string(name, item, param):
	params = [param]
	mid = make_mid_string(name, "ops", item)
	return make_adm_add_generic_string("ops", mid, params)

#
# Makes and returns the function call string for adm_add_var
# name is the name returned by the call to initialize_names(), item is the object to add,
# param_type and param_expr are the other parameters to pass to adm_add_var
#
def make_adm_add_var_string(name, item, param_type, param_expr):
	params = [param_type, param_expr]
	mid = make_mid_string(name, "var", item)
	return make_adm_add_generic_string("var", mid, params)

#
# Makes and returns the function call to oid_nn_add_parm()
#
def make_oid_nn_add_parm_str(short_name, ttype):
	add_parm_str = "\toid_nn_add_parm({0}_ADM_{1}_NN_IDX, {0}_ADM_{1}_NN_STR, \"{0}\", \"2017-08-17\");\n";

	return add_parm_str.format(short_name.upper(), ttype)

#
# Given a @definition entry from the JSON file, returns a
# (string, string, list) tuple of the type (VAR, EDD, etc),
# name, and list of parameters for the definition
#
# type.name(param1, param2,...) returns (type, name, [param1, param2,...])
# Returns an empty list for parameters if none are present
#
def parse_definition(definition):
	# Strip off the final parenthesis and split on the opening parenthesis
	definition = definition.split(')')[0]
	
	definition = definition.split('(')
	
	# Parameters are whatever is after the opening paranthesis, if it exists. basename is before
	d_type, d_name = definition[0].split('.')
	d_param_str = None
	if(len(definition) > 1):
		d_param_str = definition[1]

	# further split on commas to get individual parameters
	d_parameters = []
	if d_param_str is not None:
		d_parameters = d_param_str.split(',')

	return d_type, d_name, d_parameters

#
# Given a @ttype (EDD, VAR, META, etc), a @name for an item in that type,
# and the @data dictionary resulting from json_load(), returns a list of
# the types found in the parmspec dictionary for that item
#
def find_parm_types(ttype, name, data):
	haystack = []

	if ttype == "META":
		haystack = data.get("adm:constants", [])[:4]
	elif ttype == "EDD":
		haystack = data.get("adm:externally-defined-data", [])
	elif ttype == "VAR":
		haystack = data.get("adm:variables", [])
	elif ttype == "CTRL":
		haystack = data.get("adm:controls", [])
		
		
	for item in haystack:
		if(item["name"] == name):
			# Fancy way of pulling out all of the keys (types) in each of the parameter dictionaries
			parm_types = list(set().union(* (d.keys() for d in item.get("parmspec", []))))
			return parm_types

	return None


	
#
# Returns True if the passed value is a mapped parameter, False if not.
# Assumes that if a value is NOT in quotes, it is mapped.
# TODO: verify this assumption
#
def is_mapped_param(value):
	first_char = value[:1]
	
	if(first_char == '"' or first_char == "'"):
		return False
	return True

#
# Takes a quoted parameter and returns it in the correct type.
# Assumes that strings can be left as-is, and others just need the
# quotes removed.
#
def quoted_to_type(ttype, value):
	if(ttype == "STR"):
		return value
	return value[1:-1]

#
# Returns a simple default value for a passed type
#
def get_default_for_type(ttype):
	if ttype == "STR":
		return "\"\""
	return "0"

#
# Given a @value and a parmspec list, (@params),
# Finds and returns the index of the passed value in the list
#
def get_index_in_parmspec(value, params):
	value = value.strip()

	for index, d in enumerate(params):
		if value in d.values():
			return index

	return None

#
# Creates and returns a string for the calls necessary to add and map parameters for a
# definition of a report template
# @definition is the unparsed string value of the definition to complete parms for
# @params is the list of param dictionarys for this report
# @data is the dictionary for this JSON, created by a call to json_load
#
def make_rpt_parm_calls_string(definition, params, data):
	out = ""
	mid_add_param_str      = "mid_add_param_from_value(cur_mid, val_from_{0}({1}));\n\t"
	cur_item_stat_parm_str = "cur_item = rpttpl_item_create(cur_mid, {0});\n\t"
	add_parm_map_str       = "rpttpl_item_add_parm_map(cur_item, {1}, {0});\n\t"
	
	d_type, d_name, d_parameters = parse_definition(definition)
	mapping_dest = {}

	parm_types = []
	if len(d_parameters) > 0:
		# Need to look for the types of the parameters in their original location (EDD, VAR, etc)
		parm_types = find_parm_types(d_type, d_name, data)
		if( len(parm_types) != len(d_parameters) ) :
			print "[Error] number of parameters provided for report does not match number expected by ", d_name
			raise Exception
	
	# For each parameter in this definition
	for index, d_param in enumerate(d_parameters):
		is_mapped = is_mapped_param(d_param)
		
		# If the parameter is mapped, use the default value for now and save the mapping
		if is_mapped:
			src = get_index_in_parmspec(d_param, params)
			if src == None:
				print "Error: could not find source for parameter: ", d_param
				raise Exception
			
			mapping_dest[index] = src
			parm_value = get_default_for_type(parm_types[index])
			
		# Otherwise, parse the given value to the right type to print
		else:
			parm_value = quoted_to_type(parm_types[index], d_param)			

		# Add the mid_add_param_from_value() call for each parameter
		out = out + mid_add_param_str.format(parm_types[index].lower(), parm_value)

	# Create the rpttpl item with the correct number of mapped items
	out = out + cur_item_stat_parm_str.format(str(len(mapping_dest.keys())))

	# For each mapped parameter, add the mapping.
	# NOTICE: indices for the called c function start at 1
	for dest, src in mapping_dest.items():
		out = out + add_parm_map_str.format(str(dest+1), str(src+1))
	
	return out
