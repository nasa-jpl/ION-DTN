/********************************************************
 **  Authors: Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Federico Domenicali, federico.domenicali@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

#include "al_bp_opts_parser.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))

typedef enum{
	IPN_LOCAL=50,
	ORDINAL=51,
	UNRELIABLE=52,
	CRITICAL=53,
	FLOW=54,
	FORCE_EID=55,
	DEL=56,
	MB_TYPE=57,
	MB_STRING=58,
}bp_options_val;


static void set_default_bp_options(bundle_options_t * bundle_options){
	//options->bp_implementation = al_bp_get_implementation();
	//al_bp_get_implementation() = BP_ION;
	bundle_options->lifetime = 60;
	bundle_options->custody_reports = FALSE;
	bundle_options->custody_transfer = FALSE;
	bundle_options->deletion_reports = FALSE;
	bundle_options->delivery_reports = TRUE;
	bundle_options->disable_fragmentation = FALSE;
	bundle_options->forwarding_reports = FALSE;
	bundle_options->priority.ordinal = 0;
	bundle_options->priority.priority = BP_PRIORITY_NORMAL;
	bundle_options->reception_reports = FALSE;
	bundle_options->unreliable = FALSE;
	bundle_options->critical = FALSE;
	bundle_options->ipn_local = 0;
	bundle_options->flow_label = 0;
	bundle_options->eid_format_forced = 'N';

}

int al_bp_check_bp_options(int argc, char **argv, bundle_options_t* bundle_options, result_options_t* result_options) {
	if (argv == NULL || bundle_options == NULL || result_options == NULL)
		return WRONG_VALUE;
	
	set_default_bp_options(bundle_options);
	result_options->recognized_options.opts = (checked_bp_option_t *) malloc(sizeof(checked_bp_option_t) * argc);
	result_options->recognized_options.opts_size = 0;
	result_options->unknown_options.opts = malloc(sizeof(char *) * argc);
	result_options->unknown_options.opts_size = 0;
	int result_index=0;
	char c;
	int done=0;
	int result_value=OK;
	
	opterr=0;
	
	while(!done){
		static struct option long_options[] =
		{
				{"priority", required_argument, 0, 'p'},
				{"nofragment", no_argument, 0, 'N'},
				{"received", no_argument, 0, 'r'},
				{"forwarded", no_argument, 0, 'f'},
				{"lifetime", required_argument, 0, 'l'},
				{"ipn-local", required_argument, 0, IPN_LOCAL},
				{"ordinal", required_argument, 0, ORDINAL},
				{"unreliable", no_argument, 0, UNRELIABLE},
				{"critical", no_argument, 0, CRITICAL},
				{"flow", required_argument, 0, FLOW},
				{"custody", no_argument, 0, 'C'},
				{"force-eid", required_argument, 0, FORCE_EID},
				{"del", no_argument, 0, DEL},
				{"mb-type", required_argument, 0, MB_TYPE},
				{"mb-string", required_argument, 0, MB_STRING},
				{0, 0, 0, 0}	// The last element of the array has to be filled with zeros.

		};
		int option_index=0;

		c = getopt_long(argc, argv, "C:l:p:Nrf::", long_options, &option_index);
		switch (c){
		case 'p':
			result_options->recognized_options.opts[result_index].option = "priority";
			if (!strcasecmp(optarg, "bulk"))   {

								} else if (!strcasecmp(optarg, "normal")) {
									bundle_options->priority.priority = BP_PRIORITY_NORMAL;
									result_options->recognized_options.opts[result_index].result_code=OK;
									result_options->recognized_options.opts[result_index].option_arg = "normal";
								} else if (!strcasecmp(optarg, "expedited")) {
									bundle_options->priority.priority = BP_PRIORITY_EXPEDITED;
									result_options->recognized_options.opts[result_index].result_code=OK;
									result_options->recognized_options.opts[result_index].option_arg = "expedited";
								} else if (!strcasecmp(optarg, "reserved")) {
									bundle_options->priority.priority = BP_PRIORITY_RESERVED;
									result_options->recognized_options.opts[result_index].result_code=OK;
									result_options->recognized_options.opts[result_index].option_arg = "reserved";
								} else {
									result_options->recognized_options.opts[result_index].result_code=WRONG_VALUE;
									result_value = min(result_value, WRONG_VALUE);
									result_options->recognized_options.opts[result_index].option_arg = "wrong_value";
			}
			result_index++;
			break;
		case 'N':
			result_options->recognized_options.opts[result_index].option="nofragment";
			bundle_options->disable_fragmentation=TRUE;
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			break;
		case 'r':
			result_options->recognized_options.opts[result_index].option="received";
			bundle_options->reception_reports=TRUE;
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			break;
		case 'f':
			result_options->recognized_options.opts[result_index].option="forwarded";
			bundle_options->forwarding_reports=TRUE;
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			break;
		case 'l':
			result_options->recognized_options.opts[result_index].option = "lifetime";
			bundle_options->lifetime=atoi(optarg);
			result_options->recognized_options.opts[result_index].option_arg=optarg;
			//result_options->recognized_options.opts[result_index].option_arg=malloc(sizeof(char)*strlen(optarg));
			//strcpy(result_options->recognized_options.opts[result_index].option_arg, optarg);
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			break;
		case 'C':
			result_options->recognized_options.opts[result_index].option = "custody";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			bundle_options->custody_reports=1;
			bundle_options->custody_transfer=1;
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			break;
		case IPN_LOCAL:
			result_options->recognized_options.opts[result_index].option = "ipn-local";
			bundle_options->ipn_local = atoi(optarg);
			result_options->recognized_options.opts[result_index].option_arg=optarg;
			//result_options->recognized_options.opts[result_index].option_arg=malloc(sizeof(char)*strlen(optarg));
			//strcpy(result_options->recognized_options.opts[result_index].option_arg,optarg);
			if (bundle_options->ipn_local <= 0)
			{
				result_options->recognized_options.opts[result_index].result_code=WRONG_VALUE;
				result_value = min(result_value, WRONG_VALUE);
			} else {
				result_options->recognized_options.opts[result_index].result_code=OK;
			}
			result_index++;
			break;
		case ORDINAL:
			result_options->recognized_options.opts[result_index].option = "ordinal";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			if (al_bp_get_implementation() != BP_ION){
				result_options->recognized_options.opts[result_index].result_code=WRONG_IMPLEMENTATION;
				result_value = min(result_value, WRONG_IMPLEMENTATION);
				result_index++;
				break;
			}
			result_options->recognized_options.opts[result_index].option_arg=optarg;
			//result_options->recognized_options.opts[result_index].option_arg=malloc(sizeof(char)*strlen(optarg));
			//strcpy(result_options->recognized_options.opts[result_index].option_arg,optarg);
			bundle_options->priority.ordinal = atoi(optarg);
			if(bundle_options->priority.ordinal > 254)
			{
				result_options->recognized_options.opts[result_index].result_code=WRONG_VALUE;
				result_value = min(result_value, WRONG_VALUE);
				result_index++;
				break;
			}
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			break;
		case UNRELIABLE:
			result_options->recognized_options.opts[result_index].option = "unreliable";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			if( al_bp_get_implementation() != BP_ION){
				result_options->recognized_options.opts[result_index].result_code=WRONG_IMPLEMENTATION;
				result_value = min(result_value, WRONG_IMPLEMENTATION);
				result_index++;
				break;
			}
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			bundle_options->unreliable = TRUE;
			break;
		case CRITICAL:
			result_options->recognized_options.opts[result_index].option = "critical";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			if( al_bp_get_implementation() != BP_ION){
				result_options->recognized_options.opts[result_index].result_code=WRONG_IMPLEMENTATION;
				result_value = min(result_value, WRONG_IMPLEMENTATION);
				result_index++;
				break;
			}
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			bundle_options->critical = TRUE;
			break;
		case FLOW:
			result_options->recognized_options.opts[result_index].option = "flow";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			if( al_bp_get_implementation() != BP_ION){
				result_options->recognized_options.opts[result_index].result_code=WRONG_IMPLEMENTATION;
				result_value = min(result_value, WRONG_IMPLEMENTATION);
				result_index++;
				break;
			}
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			bundle_options->flow_label = atoi(optarg);
			break;
		case FORCE_EID:
			result_options->recognized_options.opts[result_index].option = "force-eid";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			if(strcmp(optarg,"D")){
				result_options->recognized_options.opts[result_index].option_arg= "D";
				result_options->recognized_options.opts[result_index].result_code=OK;
				bundle_options->eid_format_forced = 'D';
			}else if(strcmp(optarg,"I")){
				result_options->recognized_options.opts[result_index].option_arg="I";
				result_options->recognized_options.opts[result_index].result_code=OK;
				bundle_options->eid_format_forced = 'I';
			}else{
				result_options->recognized_options.opts[result_index].result_code=WRONG_VALUE;
				result_value = min(result_value, WRONG_VALUE);
				result_index++;
				break;
			}
			result_index++;
			break;
		case DEL:
			result_options->recognized_options.opts[result_index].option = "del";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			bundle_options->deletion_reports=TRUE;
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			break;
		case MB_TYPE:
			result_options->recognized_options.opts[result_index].option = "mb-type";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			if(al_bp_get_implementation() != BP_DTN && al_bp_get_implementation() != BP_ION)
			{
				result_options->recognized_options.opts[result_index].result_code=WRONG_IMPLEMENTATION;
				result_value = min(result_value, WRONG_IMPLEMENTATION);
				result_index++;
				break;
			}
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			bundle_options->metadata_type=atoi(optarg);
			result_options->recognized_options.opts[result_index].option_arg=optarg;
			//result_options->recognized_options.opts[result_index].option_arg=malloc(sizeof(char)*strlen(optarg));
			//strcpy(result_options->recognized_options.opts[result_index].option_arg,optarg);
			break;
		case MB_STRING:
			result_options->recognized_options.opts[result_index].option = "mb-string";
			result_options->recognized_options.opts[result_index].option_arg=NULL;
			if(al_bp_get_implementation() != BP_DTN && al_bp_get_implementation() != BP_ION)
			{
				result_options->recognized_options.opts[result_index].result_code=WRONG_IMPLEMENTATION;
				result_value = min(result_value, WRONG_IMPLEMENTATION);
				result_index++;
				break;
			}
			result_options->recognized_options.opts[result_index].result_code=OK;
			result_index++;
			bundle_options->metadata_string=optarg;
			result_options->recognized_options.opts[result_index].option_arg=optarg;
			//result_options->recognized_options.opts[result_index].option_arg=malloc(sizeof(char)*strlen(optarg));
			//strcpy(result_options->recognized_options.opts[result_index].option_arg,optarg);
			break;
		case '?':
			result_options->unknown_options.opts[result_options->unknown_options.opts_size] = argv[optind-1];
			result_options->unknown_options.opts_size++;
			if(optind!=argc){
				if(argv[optind][0]!='-'){
					result_options->unknown_options.opts[result_options->unknown_options.opts_size] = argv[optind];
					result_options->unknown_options.opts_size++;
				}
			}
			break;
		case (char)(-1):
			done=-1;
			break;
		}

	}
	result_options->recognized_options.opts_size = result_index;
	return result_value;
}

void al_bp_free_result_options(result_options_t result_options) {
	free(result_options.recognized_options.opts);
	free(result_options.unknown_options.opts);
}

char * get_help_bp_options(void){
    return "\nBundle Protocol options:\n"
			" -C, --custody               Request of custody transfer (and of \"custody accepted\" status reports as well).\n"
			" -f, --forwarded             Request of \"forwarded\" status reports.\n"
			" -r, --received              Request of \"received\" status reports.\n"
			"     --del                   Request of \"deleted\" stautus reports.\n"
			" -N, --nofragment            Disable bundle fragmentation.\n"
			"     --debug[=level]         Debug messages [1-2]; if level is not indicated level = 2.\n"
			" -l, --lifetime <time>       Bundle lifetime (s). Default is 60 s.\n"
			" -p, --priority <val>        Bundle  priority [bulk|normal|expedited|reserved]. Default is normal.\n"
			"     --force-eid <[DTN|IPN]> Force scheme of registration EID.\n"
			"     --ipn-local <num>       Set ipn local number (Use only on DTN2)\n"
			"     --ordinal <num>         ECOS \"ordinal priority\" [0-254]. Default: 0 (ION Only).\n"
			"     --unreliable            Set ECOS \"unreliable flag\" to True. Default: False (ION Only).\n"
			"     --critical              Set ECOS \"critical flag\" to True. Default: False (ION Only).\n"
			"     --flow <num>            ECOS \"flow\" number. Default: 0 (ION Only).\n"
			"     --mb-type <type>        Include metadata block and specify type.\n"
			"     --mb-string <string>    Extension/metadata block content.\n";
}


al_bp_error_t al_bp_create_bundle_with_option(al_bp_bundle_object_t *bundle, bundle_options_t bundle_options) {
	al_bp_error_t err;
	al_bp_bundle_delivery_opts_t dopts = BP_DOPTS_NONE;
	
	if (bundle == NULL)
		return BP_ENULLPNTR;
	
	if (bundle->id == NULL) { // I suppose the bundle_object is not initialized
		al_bp_bundle_create(bundle); // create the bundle
	}

	// Bundle expiration
	err = al_bp_bundle_set_expiration(bundle, bundle_options.lifetime);
	if (err != BP_SUCCESS) {
		return BP_EINVAL;
	}

	// Bundle priority
	err = al_bp_bundle_set_priority(bundle, bundle_options.priority);
	if (err != BP_SUCCESS) {
		return BP_EINVAL;
	}

	// Bundle unreliable
	err = al_bp_bundle_set_unreliable(bundle, bundle_options.unreliable);
	if (err != BP_SUCCESS) {
		return BP_EINVAL;
	}

	// Bundle critical
	err = al_bp_bundle_set_critical(bundle, bundle_options.critical);
	if (err != BP_SUCCESS) {
		return BP_EINVAL;
	}

	// Bundle flow label
	err = al_bp_bundle_set_flow_label(bundle, bundle_options.flow_label);
	if (err != BP_SUCCESS) {
		return BP_EINVAL;
	}
	
	// Delivery receipt option
	if (bundle_options.delivery_reports)
		dopts |= BP_DOPTS_DELIVERY_RCPT;

	// Forward receipt option
	if (bundle_options.forwarding_reports)
		dopts |= BP_DOPTS_FORWARD_RCPT;

	// Custody transfer
	if (bundle_options.custody_transfer && (bundle_options.critical == FALSE))
		dopts |= BP_DOPTS_CUSTODY;

	// Custody receipts
	if (bundle_options.custody_reports)
		dopts |= BP_DOPTS_CUSTODY_RCPT;

	// Receive receipt
	if (bundle_options.reception_reports)
		dopts |= BP_DOPTS_RECEIVE_RCPT;

	// Deleted receipts
	if (bundle_options.deletion_reports)
		dopts |= BP_DOPTS_DELETE_RCPT;

	//Disable bundle fragmentation
	if (bundle_options.disable_fragmentation)
		dopts |= BP_DOPTS_DO_NOT_FRAGMENT;

	//Set options
	err = al_bp_bundle_set_delivery_opts(bundle, dopts);

	return err;
}



