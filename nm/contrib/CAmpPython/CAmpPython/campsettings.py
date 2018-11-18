# JHU/APL
# Description: This is a library file for any settings that need to be
# set up globally for CAmpPython
# Modification History:
#   YYYY-MM-DD     AUTHOR            DESCRIPTION
#   ----------     ---------------   -------------------------------
#   2017-12-01     Sarah             File creation
#
#############################################################################

#
# Initializes global variables for CAmpPython
#
# firstbyte is the command line entry provided by the user
# for the firstbyte setting
#
def init(firstbyte):
	global new_way
	new_way = False
	if firstbyte != "old":
		new_way = True

