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
	filename = outpath + "/step5.sql"
	return filename

#
# Writes the calls for creating adm roots to the open file
# descriptor passed as sql_file
#
def write_insert_bp_adm(sql_file):
	insert_bp_str = (
		"-- Insert BP ADM\n"
		"CALL sp_create_adm_root_step1(\'iso.identified-organization.dod.internet.mgmt.dtnmp.agent\', "
		"\'1.3.6.1.2.3.1\', \'2B0601020301\', \'BP ADM\', @OIDID);\n"
		"CALL sp_create_adm_root_step2(@OIDID, \'BP ADM\', \'6\', @ADMID);\n\n")
	
	sql_file.write(insert_bp_str)

#
# Writes the calls to insert the atomic mids to the open file descriptor
# passed as sql_file
# edds is a list of externally defined data to include
#
def write_insert_atomic_mids(sql_file, edds):
	sql_file.write("-- Insert BP ADM Atomic MIDs\n")
	create_adm_atomic_mid_str = (
		"CALL sp_create_adm_atomic_mid(\'{0}\', \'{1}\', \'{2}\', {3}, \'\', 11, "
		"(SELECT ID FROM amp_core.lvtDataTypes WHERE NAME=\'{4}\'), @RESULT_MID_ID);\n")
	
	for i in edds:
		try:
			description = campsql.parameter_format(i["description"])
			sql_file.write(create_adm_atomic_mid_str.format(i["name"], i, camputil.convert_mid(i), description, i["type"]))
		except KeyError, e:
			print "[ Error ] Badly formatted edd. Key not found:"
			print e
			raise

	sql_file.write("\n")

#
# Writes the calls to insert the ADM report definitions to the open
# file descriptor passed as sql_file
# reports is a list of report templates to include
#
def write_insert_report_defs(sql_file, reports):
	comment_str = (
		"-- Insert Agent ADM Report Definitions\n"
		"-- 1) Create Data Collection of Data Type MID Collection containing the MID Collection.\n"
		"-- 2) Create the ADM Report MID\n")
	insert_str = (
		"INSERT INTO amp_core.dbtDataCollections (Label) VALUES (\'Report: {0} (ADM Defined)\');\n"
		"SET @DC_ID = LAST_INSERT_ID();\n"
		"INSERT INTO amp_core.dbtDataCollection (CollectionID, DataOrder, DataType, DataBlob) "
		"VALUES (@DC_ID, WHATISTHISNUMBER, WHATISTHISNUMBER, \'WHATISTHISHEX\');\n"
		"CALL sp_create_adm_report_mid(\'{0}\', \'{1}\', \'{2}\', \'{0}\', {3}, \'WHATISTHISNUMBER\', "
		"@DC_ID, @RESULT_MID_ID);\n")
	
	sql_file.write(comment_str)

	for i in reports:
		try:
			description = campsql.parameter_format(i["description"])
			sql_file.write(insert_str.format(i["name"], i, camputil.convert_mid(i), description))
		except KeyError, e:
			print "[ Error ] Badly formatted report-template. Key not found:"
			print e
			raise

	sql_file.write("\n")

#
# Writes all of the operations to set up the controls to the open
# file descriptor passed as sql_file
# controls is a list of controls to work with
#
def write_controls_operations(sql_file, controls):
	drop_create_str = (
		"-- Create a Control Proto MID for {2}\n"
		"DROP PROCEDURE IF EXISTS \'sp_create_bp_adm_{2}\';\n"
		"DELIMITER $$\n"
		"CREATE PROCEDURE sp_create_bp_adm_{2} (OUT result_adm_id INT UNSIGNED)\n"
		"BEGIN\n"
		"\tSET @ControlComment = \'Control: BP ADM {0}\';\n"
		"\tSET @ControlMIDName = \'{0}\';\n"
		"\tSET @ControlOIDValue = \'{1}\';\n"
		"\tSET @ControlMIDValue = \'{2}\';\n"
		"\tSET @ControlMIDDescription = {3};\n"
		"\t-- Create a Prototype MID collection\n")
	
	params_str_start = (
		"INSERT INTO amp_core.dbtProtoMIDParameters (Comment) VALUES (@ControlComment);\n"
		"SET @PMPC_ID = LAST_INSERT_ID();\n")
	
	single_param_str = (
		"INSERT INTO amp_core.dbtProtoMIDParameter(CollectionID,ParameterOrder,ParameterTypeID) "
		"VALUES (@PMPC_ID,UNKNOWNNUMBER,UNKNOWNNUMBER\n"
		"SET @PMP_ID= LAST_INSERT_ID();\n"
		"INSERT INTO dtnmp_app.dbtProtoMIDParameterMetadata (ParameterID, Name, Description, FieldTypeID) "
		"VALUES (@PMP_ID, \'{0}\', {1}, \'WHATISTHISNUMBER\'\n")
	
	params_str_end = (
		"\tCALL sp_create_control_proto_mid(@ControlMIDName,@ControlOIDValue,@ControlMIDValue,"
		"@ControlMIDName,@ControlMIDDescription, 14, @PMPC_ID, @RESULT_MID_ID);\n"
		"\tSET result_adm_id = @RESULT_MID_ID;\n"
		"END $$\n"
		"DELIMITER ;\n")
	
	for i in controls:
		try:
			description = campsql.parameter_format(i["description"])
			sql_file.write(drop_create_str.format(i["name"], i, camputil.convert_mid(i), description))
		
			parameters=camputil.parse_params(i.get("paramspec-proto", ""))
			if parameters != [['']]:
				sql_file.write(params_str_start)
			
				for j in parameters:
					#XXX This doesn't make sense, why iterate over j but add i?
					sql_file.write(single_param_str.format(i["name"], description))
				
				sql_file.write(params_str_end)
		except KeyError, e:
			print "[ Error ] Badly formatted control. Key not found:"
			print e
			raise

	controls_call_str = "CALL sp_create_bp_adm_{0}(@RESULT_MID_ID);\n"
	for i in controls:
		try:
			sql_file.write(controls_call_str.format(camputil.convert_mid(i)))
		except KeyError, e:
			print "[ Error ] Badly formatted report-template. Key not found:"
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
		sql_file = open(filename,'w')
	except IOError, e:
		print "[ Error ] Failed to open ", filename, " for writing."
		print e
		return

	print "Working on ", filename,
	
	campsql.write_standard_sql_file_header(sql_file)

	sql_file.write("USE dtnmp_app;\n")

	write_insert_bp_adm(sql_file)

	sql_file.write("CALL sp_create_adm_nickname(@ADMID, 18, \'BP Root\', @OIDID, @NNID);\n\n")

	try: 
		write_insert_atomic_mids(sql_file, data.get("adm:externally-defined-data", []))

		write_insert_report_defs(sql_file, data.get("adm:report-templates", []))
		# XXX this one needs to be looked at again
		#	write_controls_operations(sql_file, data.get("adm:controls", []))
	except KeyError, e:
		return
	finally:
		sql_file.close()

	print "\t[ DONE ]"
