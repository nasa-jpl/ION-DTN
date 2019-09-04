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
 * 				initialization lines to the extensionDefs[]
 * 				array, referencing the declared extension
 * 				block callback functions, and add one or
 * 				more ExtensionSpec structure initialization
 * 				lines to the extensionSpecs[] array
 * 				(referencing the new extension definition)
 * 				indication the location within the bundle
 * 				at which each instance of the newly
 * 				defined extension block should be
 * 				inserted.
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

static ExtensionDef	extensionDefs[] =
			{
				{ "unknown",0,0,{0,0,0,0,0},0,0,0,0,0,0,0,0,0 }
			};

/*	NOTE: the order of appearance of extension specifications in
 *	the extensionSpecs array determines the order in which 
 *	extension blocks will be inserted into locally sourced bundles
 *	prior to the payload block.					*/

static ExtensionSpec	extensionSpecs[] =
			{
				{ 0,0,0,0 }
			};
