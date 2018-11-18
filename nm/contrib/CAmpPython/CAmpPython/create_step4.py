# JHU/APL
# Description: creates the sql file 
# Modification History:
#   YYYY-MM-DD     AUTHOR            DESCRIPTION
#   ----------     ---------------   -------------------------------
#   2017-12-19     Sarah             Added this header comment
#
###############################################

import re
import datetime
import os

import camputil
import campsql


#
# Creates and returns the filename for the generated file
#
def initialize_filename(outpath):
	filename = outpath + "/step4.sql"
	return filename

#
# Writes the insert agent ADM calls to the open file descriptor passed
# as sql_file
#
def write_insert_agent_adm(sql_file):
	insert_agent_str = (
    		"\n-- Insert Agent ADM"
		"\nCALL sp_create_adm_root_step1(\'iso.identified-organization.dod.internet.mgmt.dtnmp.agent\', "
		"\'1.3.6.1.2.3.3\', '\2B0601020303\', \'Agent ADM\', @OIDID);"
		"\nCALL sp_create_adm_root_step2(@OIDID, \'DTNMP Agent ADM\', \'2016-06-29\',@ADMID);"
		"\n\n")
	sql_file.write(insert_agent_str)

#
# Writes the calls for inserting agent nicknames to the open file
# descriptor passed as sql_file
#
def write_insert_agent_nicknames(sql_file):
    	sql_file.write("-- Insert Agent Nicknames\n")
	create_adm_nickname_str = "CALL sp_create_adm_nickname(@ADMID, {0}, \'Agent {1}\', @OIDID, @NNID);\n"
	nicknames = ["Metadata", "Externally Defined Data", "Variable Defs", "Reports", "Controls",
		     "Constants", "Macros", "Operators", "Root"]
	
	# Formats the create_adm_nickname_str index and value of each of the items in the
	# nicknames list
	for idx, name in enumerate(nicknames):
		sql_file.write(create_adm_nickname_str.format(idx, name))

    	sql_file.write("\n\n")

#
# Writes the calls for inserting agent ADM metadata to the open file
# descriptor passed as sql_file
# metadata is a list of metadata to include
#
def write_insert_adm_metadata(sql_file, metadata):
	metadata_str = (
		"CALL sp_create_adm_atomic_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, "
		"(SELECT ID FROM amp_core.dbtADMNicknames WHERE amp_core.dbtADMNicknames.Nickname_UID = 1), "
		"(SELECT ID FROM amp_core.lvtDataTypes WHERE NAME=\'{4}\'), @RESULT_MID_ID);"
		"\n")
	
    	sql_file.write("-- Insert Agent ADM Metadata\n")
	
    	for i in metadata:
		try:
			description = campsql.parameter_format(i["description"])
			sql_file.write(metadata_str.format(i["name"], i, camputil.convert_mid(i), description, i["type"]))
		except KeyError, e:
			print "[ Error ] Badly formatted metadata. Key not found:"
			print e
			raise

    	sql_file.write("-- SELECT @RESULT_MID_ID;\n\n\n")

#
# Writes the calls for inserting agent ADM operators to the open file
# descriptor passed as sql_file
# operators is a list of operators to include
#
def write_insert_adm_operator(sql_file, operators):
	operator_str = (
		"CALL sp_create_adm_operator_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, "
		"(SELECT ID FROM amp_core.dbtADMNicknames WHERE amp_core.dbtADMNicknames.Nickname_UID = 7), "
		"@RESULT_MID_ID);"
		"\n")

    	sql_file.write("-- Insert Agent ADM Operator Definition\n")

    	for i in operators:
		try:
			description = campsql.parameter_format(i["description"])
			sql_file.write(operator_str.format(i["name"], i, camputil.convert_md(i), description))
		except KeyError, e:
			print "[ Error ] Badly formatted operator. Key not found:"
			print e
			raise

    	sql_file.write("\n\n")

#
# Writes the calls for inserting ADM computed data definitions to the
# open file descriptor passed as sql_file
# variables is a list of variables to include
#
def write_insert_adm_datadef(sql_file, variables):
	comment_str = (
    		"-- Insert Agent ADM Computed Data Definitions\n"
    		"-- Computed Data definitions require the creation of the following: \n"
		"-- 1) Creation of a MID Parameters collection\n"
    		"-- 2) Population of a MID Parameter dataset containing a Data Collection of Data Type 'Variable'\n"
    		"-- 3) Creation of a Computed Data MID\n"
    		"--\n"
    		"-- Steps:\n"
    		"-- 1) Create Data Collection of Data Type MID Collection containing the MID Collection.\n"
    		"-- 2) Create the computed data MID\n")
	
	insert_str = (
		"INSERT INTO amp_core.dbtDataCollections (Label) VALUES (\'Expression: {0}\');\n"
		"SET @DC_ID= LAST_INSERT_ID();\n"
    		"INSERT INTO amp_core.dbtDataCollection (CollectionID, DataOrder, DataType, DataBlob) "
		"VALUES (@DC_ID, 1,(SELECT ID FROM amp_core.lvtDataTypes WHERE NAME=\'EXPR\'), \'WHAT IS THIS VALUE\');"
		"\n"
		"CALL sp_create_adm_computed_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, "
		"(SELECT ID FROM amp_core.dbtADMNicknames WHERE amp_core.dbtADMNicknames.Nickname_UID=2), @DC_ID, @RESULT_MID_ID);"
		"\n")
		
	sql_file.write(comment_str)
		
    	for i in variables:
		try:
			description = campsql.parameter_format(i["description"])
			sql_file.write(insert_str.format(i["name"], i, camputil.convert_mid(i), description))
		except KeyError, e:
			print "[ Error ] Badly formatted variable. Key not found:"
			print e
			raise

    	sql_file.write("\n\n")

#
# Writes the calls for inserting ADM report definitions to the open
# file descriptor passed as sql_file
# reports is a list of reports to include
#
def write_insert_adm_reportdef(sql_file, reports):
	comment_str = (
		"-- Insert Agent ADM Report Definitions\n"
    		"-- 1) Create Data Collection of Data Type Mid Collection containing the MID Collection.\n"
    		"-- 2) Create the ADM Report MID")
	
	insert_str = (
		"INSERT INTO amp_core.dbtDataCollections (Label) VALUES (\'Report: {0}\');\n"
    		"SET @DC_ID = LAST_INSERT_ID();"
		"\n"
    		"INSERT INTO amp_core.dbtDataCollection (CollectionID, DataOrder, DataType, DataBlob) "
		"VALUES (@DC_ID,1, (SELECT ID FROM amp_core.lvtDataTypes WHERE NAME=\'MC\'),\'WHAT IS THIS VALUE\');"
		"\n"
    		"CALL sp_create_adm_macro_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, "
		"(SELECT ID FROM amp_core.dbtADMNicknames "
		"WHERE amp_core.dbtADMNicknames.Nickname_UID = 3), @DC_ID, @RESULT_MID_ID);"
		"\n")

    	for i in reports:
		try:
			description = campsql.parameter_format(i["description"])
			sql_file.write(insert_str.format(i["name"], i, camputil.convert_mid(i), description))
		except KeyError, e:
			print "[ Error ] Badly formatted report-template. Key not found:"
			print e
			raise

    	sql_file.write("\n\n")

#
# Writes the calls to insert the macros to the open file descriptor passed
# as sql_file
# macros is a list of macros to include
#
def write_insert_macros(sql_file, macros):
	comment_str = (
    		"-- Macro\n"
    		"-- 1) Create Data Collection of Data Type MID Collection containing the MID Collection\n"
    		"-- 2) Create the ADM Report MID\n")

	insert_str = (
		"INSERT INTO amp_core.dbtDataCollections (Label) VALUES (\'Macro: {0}\');\n"
    		"SET @DC_ID = LAST_INSERT_ID();\n"
		"INSERT INTO amp_core.dbtDataCollection (CollectionID, DataOrder, DataType, DataBlob) "
		"VALUES (@DC_ID,1, (SELECT ID FROM amp_core.lvtDataTypes WHERE NAME=\'MC\'), \'WHAT IS THIS VALUE\');\n"
		"CALL sp_create_adm_macro_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, \'WHAT IS THIS VALUE\', "
		"@DC_ID, @RESULT_MID_ID);\n")
		
    	for i in macros:
		try:
			description = campsql.parameter_format(i["description"])
			sql_file.write(insert_str.format(i["name"], i, camputil.convert_mid(i), description))
		except KeyError, e:
			print "[ Error ] Badly formatted macro. Key not found:"
			print e
			raise

    	sql_file.write("\n\n")

#
# Writes the calls to insert the constants to the open file descriptor passed
# as sql_file
# constants is a list of constants to include
#
def write_insert_constants(sql_file, constants):
    	sql_file.write("-- Constants\n")

	first_str = (
		"CALL sp_create_adm_literal_atomic_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, "
		"(SELECT ID FROM amp_core.dbtADMNicknames WHERE amp_core.dbtADMNicknames.Nickname_UID = 5), @RESULT_MID_ID);\n")
	
	param_str = (
		"INSERT INTO amp_core.dbtProMIDParameters (Comment) VALUES (\'Constant: {0}\');\n"
		"SET @PMPC_ID = LAST_INSERT_ID();\n"
		"INSERT INTO amp_core.dbtProtoMIDParameter(CollectionID, ParameterOrder,ParameterTypeID) "
		"VALUES (PMPC_ID,1,(SELECT ID FROM amp_core.lvtDataTypes WHERE NAME=\'{0}\'));\n"
		"CALL sp_create_literal_proto_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, "
		"(SELECT ID FROM amp_core.dbtADMNicknames WHERE amp_core.dbtADMNicknames.Nickname_UID = 5), @PMPC_ID, "
		"(SELECT ID FROM amp_core.lvtDataTypes WHERE NAME=\'{0}\'), @RESI:T_MID_ID);\n")
	
    	for i in constants:
		try:
			description = campsql.parameter_format(i["description"])
    			if i==0:
				sql_file.write(first_str.format(i["name"], i, camputil.convert_mid(i), description))
			else:	
				parameters=camputil.parse_params(i.get("paramspec-proto", ""))
				if parameters !=[['']]:
					for j in parameters:
						sql_file.write(param_str.format(i["name"], i, camputil.convert_mid(i), description))
		except KeyError, e:
			print "[ Error ] Badly formatted constant. Key not found:"
			print e
			raise
		
	sql_file.write("\n\n")
	
#
# Writes the callse to insert the controls to the open file descriptor
# passed as sql_file
# controls is a list of controls to include
#
def write_insert_controls(sql_file, controls):
	sql_file.write("-- Control\n")
	sql_file.write("-- Create Prototype MID Collection\n")

	control_str = (
		"INSERT amp_core.dbtProtoMIDParameters (Comment) VALUES (\'Control: {0}\');\n"
		"SET @PMPC_ID = LAST_INSERT_ID();\n\n")

	param_str = (
		"INSERT INTO amp_core.dbtProtoMIDParameter(CollectionID, ParameterOrder, ParameterTypeID) "
		"VALUES (PMPC_ID,{0},(SELECT ID FROM amp_core.lvtDataTypes WHERE NAME=\'{1}\'));\n"
		"SET PMP_ID = LAST_INSERT_ID();\n\n")

	control_proto_str = (
		"CALL sp_create_control_proto_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, "
		"(SELECT ID FROM amp_core.dbtADMNicknames "
		"WHERE amp_core.dbtADMNicknames.Nickname_UID = 4), @PMPC_ID, @RESULT_MID_ID);\n\n")

	for i in controls:
		try:
			sql_file.write(control_str.format(i["name"]))

			parameters=camputil.parse_params(i.get("paramspec-proto", ""))
			if parameters != [['']]:
				for j in parameters:
					# XXX: This used to be j, j["name"], but j is not a dictionary. 
					sql_file.write(param_str.format(j[0], j[1]))
					
				description = campsql.parameter_format(i["description"])
				sql_file.write(control_proto_str.format(i["name"], i, camputil.convert_mid(i), description))
		except KeyError, e:
			print "[ Error ] Badly formatted control. Key not found:"
			print e
			raise
					
#
# Main function of this file, which calls out to helper functions to
# orchestrate the creation of its generated file
# data is the dictionary created from the parsed JSON
# outpath is the path to the output directory
#
def create(data, outpath):
	filename = initialize_filename(outpath)

	# Open the file for writing
	try: 
		sql_file = open(filename, "w")
	except IOError, e:
		print "[ Error ] Failed to open ", filename, " for writing."
		print e
		return

	print "Working on ", filename,
	
	campsql.write_standard_sql_file_header(sql_file)

    	sql_file.write("USE dtnmp_app;\n\n")

	write_insert_agent_adm(sql_file)

	write_insert_agent_nicknames(sql_file)

	try: 
		write_insert_adm_metadata(sql_file, data.get("adm:metadata", []))

		write_insert_adm_operator(sql_file, data.get("adm:operators", []))

		write_insert_adm_datadef(sql_file, data.get("adm:variables", []))

		write_insert_adm_reportdef(sql_file, data.get("adm:report-templates", []))
		
		write_insert_macros(sql_file, data.get("adm:macros", []))

		write_insert_constants(sql_file, data.get("adm:constants", []))

		write_insert_controls(sql_file, data.get("adm:controls", []))
	except KeyError, e:
		return
	finally:
		sql_file.close()
	
	print "\t[ DONE ]"






