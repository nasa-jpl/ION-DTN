#ionstart.awk
# David Young
# 20 AUG 2008
#
# This awk script takes, from standard input, a text configuration file.
# The file will contain configuration commands for several ion node
# administration programs.
# Each section will be dilineated by a pair of Marker lines:
## begin programname 
# 	This line will appear at the start of a section.
## end programname
#	This line will appear at the end of a section.
#
# An option is permitted: tag
# Defining "tag" will limit which begin and end lines will match.
# For instance, you can have 2 node's configuration commands in different
# sets of sections, such as:
## begin ionadmin host1
## begin ionadmin host2
# the tag will allow the command to consider only host1's lines.
# A side effect of this is that section coherence will not be checked with
# files containing multiple tags.  This could mean that "host1"'s ionadmin
# section could overlap with "host2"'s bpadmin section, causing errors.
# BUT it can also mean that host1 and host2's ionadmin files both overlap
# completely, saving the user typing time by sending the same topology
# information to both nodes with the same file.
#
# program names accepted are:
# ionadmin ionsecadmin bpsecadmin ltpsecrc ltpadmin bpadmin cfdpadmin ipnadmin bibeadmin dtn2admin dtpcadmin acsadmin imcadmin bssadmin
#
# Program sections may not overlap.
# Lines with unsupported program names will be ignored.
# In the case of an end line immediately following a begin line
# (where there are no lines in between) will result in the program
# not being called.
#
# Optional variable "echo" will, if set, create files for each
# section matching the current tag. It will create the following:
# configfile.tag.ionrc
# configfile.tag.ionsecrc
# configfile.tag.bpsecrc
# configfile.tag.ltpsecrc
# configfile.tag.ltprc
# configfile.tag.bprc
# configfile.tag.cfdprc
# configfile.tag.ipnrc
# configfile.tag.dtn2rc
# configfile.tag.dtpcrc
# configfile.tag.acsrc
# configfile.tag.imcrc
# configfile.tag.bssrc
# it will NOT check for the file existence beforehand.
# it will NOT run the program.
#
# SCRIPT REQUIRES A VARIABLE TO BE SET: configfile
# SCRIPT HAS OPTIONAL VARIABLES: tag echo

# initialize variables
BEGIN {
	ION_OPEN_SOURCE=1
	# linenumber for reporting syntax errors and helping out the sed call
	linenumber = 0
	# programs lists the accepted packages in ion in the order they should
	# be executed
	programs[1]   = "ionadmin"
	programs[2]   = "ionsecadmin"
	programs[3]   = "bpsecadmin"
	programs[4]   = "ltpsecadmin"
	programs[5]   = "ltpadmin"
	programs[6]   = "bpadmin"
	programs[7]   = "cfdpadmin"
	programs[8]   = "ipnadmin"
	programs[9]   = "bibeadmin"
	programs[10]  = "dtn2admin"
	programs[11]  = "dtpcadmin"
	programs[12]  = "acsadmin"
	programs[13]  = "imcadmin"
	programs[14]  = "bssadmin"

	# programoptions are special options for certain programs that take them
	# rcname is the name of an rc file associated with the program
	rcname["ionadmin"]     = ionrc
	rcname["ionsecadmin"]  = ionsecrc
	rcname["bpsecadmin"]   = bpsecrc
	rcname["ltpsecadmin"]  = ltpsecrc
	rcname["bpadmin"]      = bprc
	rcname["cfdpadmin"]    = cfdprc
	rcname["ipnadmin"]     = ipnrc
	rcname["bibeadmin"]    = biberc
	rcname["dtn2admin"]    = dtn2rc
	rcname["dtpcadmin"]    = dtpcrc
	rcname["ltpadmin"]     = ltprc
	rcname["acsadmin"]     = acsrc
	rcname["imcadmin"]     = imcrc
	rcname["bssadmin"]     = bssrc

	# firstline is associative array of the "first line" for a program
	# lastline is associative array of the "last line" for a program
	# currentsection is the state of the program section we are in
	currentsection = ""
	# error tells if there are any fatal errors
	error = 0
	# warn tells if there are any warnings errors
	warn = 0
	if (configfile == "") {
		print "The variable configfile has not been defined."
		print "\tRun again with option -v configfile=filename"
		exit 1
	}
}

# keep track of the line number
{ linenumber++ }
#{ print "line " linenumber " section " currentsection }

#lines with the s command in bpadmin
#/^s/ {
	# if we are currently in bpadmin section, this is a warning.
#	if (currentsection == "bpadmin") {
#		print "Warning: Line " linenumber " contains the start command for bpadmin."
#		print "\tIf dtn2admin/ipnadmin are run as separate programs after bpadmin,"
#		print "\tthere may be warnings/errors when this command is run."
#		print "\tDisregard this warning if dtn2admin/ipnadmin are run with"
#		print "\tthe \"r\" commands in bpadmin."
#		warn++
#		next
#	}
#}

# lines that start with begin
/^## begin/ {
	# ignore if there is no program
	if ( $3 == "" ) next
	# check that the program in this line is part of ION
	exists = 0
	for (n in programs) {
		if ($3 == programs[n]) exists = 1
	}
	if (exists == 0) {
		print "WARNING: Line " linenumber " contains unaccepted program, \"" $3 "\"."
		printf ("\tAccepted programs are: ");
		for (n in programs) {
			printf ("%s ",programs[n])
		}
		print "\n\tThis line will be ignored."
		warn++
		next
	}
	# if the 4th item matches the tag 
	# or it doesn't exist and there is no tag
	# then the line has effect
	if ( $4 == tag ) {
		# process the line as a starting point

		# if we are currently in a section, this is an error
		if (currentsection != "") {
			print "ERROR: Line " linenumber " begins a new section \"" $3 "\" within section \"" currentsection "\"."
			print "\tNo nested sections."
			error++
			next
		}
		# if this section was already defined, this is an error
		if (firstline[$3] != "") {
			print "ERROR: Line " linenumber " begins section \"" $3 "\" which has already been defined at line " firstline[$3] "."
			error++
			next
		}
		# set this starting line
		# offset by one to not send the degin line itself.
		currentsection = $3
		firstline[currentsection] = linenumber + 1
		next
	}
	# this line doesn't really have any effect, but it should be considered
	# a random line starting a section that doesn't match the current tag is ignored
	if (currentsection == "") next
	if (currentsection == $3) {
		print "WARNING: Line " linenumber " begins a section for \"" $3 "\" for a different tag."
		print "\tThere is the possibility that you will send the same commands to two"
		print "\tdifferent tags: \"" $4 "\" and \"" tag "\"."
		warn++
		next
	}
	# the program noted is different from the current section
	# this should be an error- because it is possible to send incompatible
	# commands to 2 different programs
	print "ERROR: Line " linenumber " begins a new section \"" $3 "\" within section \"" currentsection "\"."
	print "\tEven though the tag doesn't match, overlapped commands may be sent to"
	print "\ttwo different, incompatible programs."
	error++
	next
}

# lines that start with end 
/^## end/ {
	# ignore if there is no program
	if ( $3 == "" ) next
	# check that the program in this line is part of ION
	exists = 0
	for (n in programs) {
		if ($3 == programs[n]) exists = 1
	}
	if (exists == 0) {
		print "WARNING: Line " linenumber " contains unaccepted program, \"" $3 "\"."
		printf ("\tAccepted programs are: ");
		for (n in programs) {
			printf ("%s ",programs[n])
		}
		print ".\n\tThis line will be ignored."
		warn++
		next
	}
	# if the 4th item matches the tag 
	# or it doesn't exist and there is no tag
	# then the line has effect
	if ( $4 == tag ) {
		# process the line as a starting point

		# if we are not currently in a section, this is an error
		if (currentsection == "") {
			print "ERROR: Line " linenumber " ends section \"" $3 "\" without a valid begin section."
			error++
			next
		}
		# if we in a different section, this is an error
		if (currentsection != $3) {
			print "ERROR: Line " linenumber " ends section \"" $3 "\" within section \"" currentsection "\"."
			print "\tNo nested sections."
			error++
			next
		}
		# if this section was already defined, this is an error
		if (lastline[$3] != "") {
			print "ERROR: Line " linenumber " ends section \"" $3 "\" which has already been defined at line " lastline[currentsection] "."
			error++
			next
		}
		# set this ending line
		# offset by one to not send the end line itself
		lastline[currentsection] = linenumber - 1
		# clear the current section
		currentsection = ""
		next
	}
	# this line doesn't really have any effect, but it should be considered
	# a random line ending a section that doesn't match the current tag is ignored
	if (currentsection == "") next
	if (currentsection == $3) {
		print "WARNING: Line " linenumber " ends a section for \"" $3 "\" but for a different tag."
		print "\tThere is the possibility that you will send the same commands to two"
		print "\tdifferent tags: \"" $4 "\" and \"" tag "\"."
		warn++
		next
	}
	# the program noted is different from the current section
	# this should be an error- because it is possible to send incompatible
	# commands to 2 different programs
	print "ERROR: Line " linenumber " ends a new section \"" $3 "\" within section \"" currentsection "\"."
	print "\tEven though the tag doesn't match, overlapped commands will be sent to"
	print "\ttwo different, incompatible programs."
	error++
	next
}

# end script calls the programs when necessary
END {
	# if we are still in a section, this is an error
	if (currentsection != "") {
		print "ERROR: File ends without ending section \"" currentsection "\"."
		error++
	}
	print "There were " warn " warning(s) and " error " error(s) in your config file."
	if ( error > 0 ) {
		print "ION node startup will not be attempted"
		exit 1
	}

	if(firstline["bssadmin"] > 0 && firstline["ipnadmin"]>0){
		print "\nError: bss and ipn are mutually exclusive!"
		exit 1
	}

	print "Sanity check of file \"" configfile "\" has been cleared."
	# start the programs
	# a firstline/lastline with an undefined value is = 0 = "" so you must check that the
	# firstline has a value > 0.
	# ignore sections with first and last lines equal (could be undefined, or could be an
	# empty entry.
	# ignore sections with last line one greater than first line.

	# run programs in order- but only if they have defined linenumbers
	for (x = 1; x <= 10; x++) {
		if (firstline[programs[x]] > 0 && 
		    firstline[programs[x]] <= lastline[programs[x]]) {
			if(ION_OPEN_SOURCE==0 && programs[x]=="cfdpadmin"){
				print "\nSkipping CFDP section. CFDP is not supported."
				continue
			}
			if (echo == "") {
				# if ipnadmin/dtn2admin are run as separate sections, then bpadmin should be run again later with the "s" command
#				if (programs[x] == "ipnadmin" || programs[x] == "dtn2admin") runlater = 1
				print "\nRunning " programs[x] " using input lines " firstline[programs[x]] " through " lastline[programs[x]] ""
				if (0 != system (" sed -n '" firstline[programs[x]] "," lastline[programs[x]] "p' <\"" configfile "\" >iontemprun ")) {
					print "Could not create temporary file iontemprun in this directory; this program will not be run."
				} else {
					if (0 != system(programs[x] " iontemprun")) {
						print "Program: " programs[x] " exited in error."
					}
					system("rm iontemprun" )
					system("sleep 1" )
				}
			} else {
				if (append == "TRUE") {
					appendop=">>"
					print "\nAppending to " rcname[programs[x]] " using input lines " firstline[programs[x]] " through " lastline[programs[x]] "."
				} else {
					appendop=">"
					print "\nCreating " rcname[programs[x]] " using input lines " firstline[programs[x]] " through " lastline[programs[x]] "."
				}
				#use sed to print the lines that matter
				#use grep to print out lines that don't begin with "##"
				if (0 != system (" sed -n '" firstline[programs[x]] "," lastline[programs[x]] "p' <\"" configfile "\" | grep -v \"^##\" " appendop "\"" rcname[programs[x]] "\"")) {
					print "Could not write to " rcname[programs[x]] " in this directory"
				} 
			}
		}
	}
#	if (runlater == 1) {
#		print("\nRunning bpadmin again to start the node.")
#		if (0 != system("echo \"s\" | bpadmin")) {
#			print "Program: bpadmin exited in error."
#		}
#	}
}
