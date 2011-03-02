#!/usr/bin/perl

if ($#ARGV == -1) {
	print "Include file size as 2^n\n";
	exit;
}

if ($ARGV[0] > 23) {
	print "Cannot do exponent higher than 23..";
	exit;
}

`touch tempfile.swap`;
`touch hugefile.big`;
`echo "0" > hugefile.big`;


for ($i=1;$i<$ARGV[0];$i++) {
	`cat hugefile.big > tempfile.swap`;
	`cat tempfile.swap >> hugefile.big`;
}

`rm tempfile.swap`;


