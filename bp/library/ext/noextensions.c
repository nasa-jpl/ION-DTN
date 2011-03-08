/*
 *	noextensions.c:	prototype for Bundle Protocol extensions
 *			definition module.
 *
 *			This file implements Bundle Protocol with
 *			no extensions.  To create an extended
 *			implementation of Bundle Protocol:
 *
 *			1.	Specify -DBP_EXTENDED in the Makefile's
 *				compiler command line when building
 *				the libbpP.c library module.
 *
 * 			2.	Create a copy of this prototype
 * 				extensions file, named "bpextensions.c",
 * 				in a directory that is made visible to
 * 				the Makefile's libbpP.c compilation
 * 				command line (by a -I parameter).
 *
 * 			3.	In the "external function declarations"
 * 				area indicated below, add "extern"
 * 				function declarations identifying the
 * 				functions that will implement your
 * 				extension(s).
 *
 * 			4.	Add one or more ExtensionDef structure
 * 				initialization lines to the extensions[]
 *				array, referencing those declared
 *				functions.
 *
 * 			5.	Develop the implementations of those
 * 				functions in one or more new source
 * 				code files.
 *
 * 			6.	Add the object file(s) for the new
 * 				extension implementation source file(s)
 * 				to the Makefile's libbpP.so link command
 * 				line.
 *
 *	Copyright (c) 2008, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

/*	Add external function declarations between here...		*/

/*	... and here.							*/

static ExtensionDef	extensions[] =
			{
				{ "unknown",0,0,0,0,0,0,0,0,0,{0,0,0,0,0} }
			};

/*	NOTE: the order of appearance of extension definitions in the
 *	extensions array determines the order in which pre-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	prior to the payload block and the order in which post-payload
 *	extension blocks will be inserted into locally sourced bundles
 *	after the payload block.					*/
