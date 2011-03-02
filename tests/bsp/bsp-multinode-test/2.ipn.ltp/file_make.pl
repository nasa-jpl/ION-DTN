#!/usr/bin/perl

if ($#ARGV == -1) {
	print "Must include size of file in bytes...\n";
	exit;
}

print "Creating a file of $ARGV[0] bytes... ";


`rm -f largefile.dmp`;


# TODO - Get back to this later

`touch largefile.dmp`;

for ($i=0; $i < $ARGV[0]/10; $i++)
{
	`echo "---- ----" >> largefile.dmp`;
}

print "Check largefile.dmp\n";
