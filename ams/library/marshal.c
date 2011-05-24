/*
 *	marshal.c:	AAMS message marshaling and unmarshaling
 *			functions definition module.
 *
 *	Copyright (c) 2010, California Institute of Technology.
 *	ALL RIGHTS RESERVED.  U.S. Government Sponsorship
 *	acknowledged.
 *
 *	Author: Scott Burleigh, JPL
 */

/*	Add external function declarations between here...		*/

/*	... and here for all marshaling and unmarshaling functions.	*/

static MarshalRule	marshalRules[] =
			{
				{ "default", NULL }
			};

static int		marshalRulesCount = sizeof marshalRules
				/ sizeof(MarshalRule);

static UnmarshalRule	unmarshalRules[] =
			{
				{ "default", NULL }
			};

static int		unmarshalRulesCount = sizeof unmarshalRules
				/ sizeof(UnmarshalRule);
