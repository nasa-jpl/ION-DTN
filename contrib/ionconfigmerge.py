#!/usr/bin/env python

# ionconfigmerge.py

# Copyright (Unpublished--all rights reserved under the copyright laws of the
# United States), U.S. Government as represented by the Administrator of the
# National Aeronautics and Space Administration. No copyright is claimed in the
# United States under Title 17, U.S. Code.

# Permission to freely use, copy, modify, and distribute this software and its
# documentation without fee is hereby granted, provided that this copyright
# notice and disclaimer of warranty appears in all copies.

# DISCLAIMER:

# THE SOFTWARE IS PROVIDED 'AS IS' WITHOUT ANY WARRANTY OF ANY KIND, EITHER
# EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, ANY WARRANTY
# THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS, ANY IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND FREEDOM FROM
# INFRINGEMENT, AND ANY WARRANTY THAT THE DOCUMENTATION WILL CONFORM TO THE
# SOFTWARE, OR ANY WARRANTY THAT THE SOFTWARE WILL BE ERROR FREE. IN NO EVENT
# SHALL NASA BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, DIRECT,
# INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR
# IN ANY WAY CONNECTED WITH THIS SOFTWARE, WHETHER OR NOT BASED UPON WARRANTY,
# CONTRACT, TORT , OR OTHERWISE, WHETHER OR NOT INJURY WAS SUSTAINED BY PERSONS
# OR PROPERTY OR OTHERWISE, AND WHETHER OR NOT LOSS WAS SUSTAINED FROM, OR AROSE
# OUT OF THE RESULTS OF, OR USE OF, THE SOFTWARE OR SERVICES PROVIDED HEREUNDER.

# This script makes merging ION config files with ionscript a bit easier.
# Provide this script with the location of ionscript, the "master" name of the *.rc files,
# and the location of those files, and it will merge all available files into a single .rc file.
# NOTE: This script can only *merge* files, not split an existing .rc file.

# Example ./ionconfigmerge ~/ion-open-source/ionscript host1 ~/ion-open-source/tests/host1test/

import sys
import os.path
import os
import subprocess

if len(sys.argv) < 4:
    sys.exit('Usage: {0} </path/to/ionscript> <rc filename> </path/to/rc files/>'.format(sys.argv[0]))

rcTypes = {'acsrc':'-a','bprc':'-b','bssrc':'-B','dtn2rc':'-d','dtpcrc':'-D','ionrc':'-i','ltprc':'-l','imcrc':'-m','ipnrc':'-p','ionsecrc':'-s','cfdprc':'-c'}

ionScriptPath = sys.argv[1]
rcName = sys.argv[2]
rcPath = sys.argv[3]

# The user could potentially pass three types of strings for ionscript:
# (1) a path directly to ionscript (ends with "ionscript") or
# a directory path (2) with or (3) without a trailing slash.

# The code below is meant to handle all three cases (hopefully)

# If the path ends with "ionscript", assume it's correct
if ionScriptPath.endswith('ionscript'):
    pass
# Else path given is a directory that may need fixing
elif os.path.isdir(ionScriptPath):
    if not ionScriptPath.endswith('/'):
        ionScriptPath += '/'
    ionScriptPath += 'ionscript'

# Sanity check
if (os.path.isfile(ionScriptPath)) and (os.access(ionScriptPath, os.X_OK)):
    print 'Found ionscript at {0}'.format(ionScriptPath)
else:
    sys.exit('ERROR: ionscript not found')

if not rcPath.endswith('/'):
    rcPath += '/'

print 'Running ionscript on {0} ION config files in {1} ...'.format(rcName,rcPath)

ionScriptCmd = []

for rcExt, rcSwitch in rcTypes.iteritems():
    rcFilePath = '{0}{1}.{2}'.format(rcPath,rcName,rcExt)
    if os.path.isfile(rcFilePath):
        ionScriptCmd.append(rcSwitch)
        ionScriptCmd.append(rcFilePath)

# If the list is empty here, the above loop didn't find any files
if not ionScriptCmd:
    sys.exit('ERROR: No ION .*rc files found')

ionScriptCmd.insert(0, ionScriptPath)
ionScriptCmd.append('-O')
ionScriptCmd.append('{0}{1}.rc'.format(rcPath,rcName))

#print ionScriptCmd # DEBUG
subprocess.call(ionScriptCmd)
