# JHU/APL
# Description: library file for common functions related to the generation of
# sql files
# Modification History:
#   YYYY-MM-DD     AUTHOR            DESCRIPTION
#   ----------     ---------------   -------------------------------
#   2017-12-18     Sarah             File creation
#
##############################################################################

import re

#
# Writes the standard sql file header to the file
# sql_file: open file descriptor to write to
#
def write_standard_sql_file_header(sql_file):
	file_header_str = (
		"-- Title: DTN Management Protocol database schema\n"
		"-- Description: Database schema for DTN Management Protocol based on\n"
  		"--              the Internet-Draft \"Delay Tolerant Network Management Protocol\n"
    		"--              draft-irtf-dtnrg-dtnmp-01\"\n"
    		"--              (https://tools.ietf.org/html/draft-irtf-dtnrg-dtnmp-01).\n"
    		"-- Conventions: \n"
    		"--             \"lvt\" = \"Limited Value Table\": This table is intended to have a\n"
    		"--              set of known values that are not intended to be\n"
    		"--              modified by users on a regular basis.\n"
    		"--             \"dbt\" = \"DataBase Table\": This table is intended for almost all\n"
    		"--              conventional database transactions a user, application,\n"
    		"--              or system may participate in on a regular basis.\n"
    		"\n"
    		"\n")
	sql_file.write(file_header_str)

#
# Takes the passed in string, and escapes any single quotes
# and adds \' to the beginning and end of the string in an
# effort to make it safe for passing as a string parameter
# to a sql function.
# tainted is the string to reformat, returns the result
#
def parameter_format(tainted):
	tainted = re.sub("\'", "\\'", tainted)
	untainted = "\'" + tainted + "\'"
	return untainted
