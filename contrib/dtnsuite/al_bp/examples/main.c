#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/file.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/stat.h>

#include "../src/al_bp_extB.h"
#include "../src/al_bp_opts_parser.h"

int main(int argc, char** argv) {
	al_bp_extB_init('N', 100);

	al_bp_extB_registration_descriptor rd;
	
	// This function registers a new connection of the application to the BP.
	printf("%d\n", al_bp_extB_register(&rd, "echo", 3));
	
	// This function returns the local EID structure associated to the registration descriptor given in input.
	al_bp_endpoint_id_t source = al_bp_extB_get_local_eid(rd);
	
	printf("Registered as %s - n.%d\n", source.uri, rd);
	
	al_bp_bundle_object_t bundle;
	bundle_options_t bundle_options;
	result_options_t result_options;
	

	for (int i = 0; i < 1; i++) {
		// This function initialize a bundle object
		al_bp_bundle_create(&bundle);
		// This function receives a bundle object from the registration_descriptor passed in input.
	
		// Send back the bundle object after setting all options passed as parameters

		al_bp_endpoint_id_t dest;
		dest = bundle.spec->source;
		
		strcpy(dest.uri, "ipn:100.1");
	
		al_bp_endpoint_id_t none;
		al_bp_get_none_endpoint(&none);

		bundle.spec->expiration = 10;
		
		bundle.payload->location = BP_PAYLOAD_MEM;
		
		char* stringa = "ciao";
		
		bundle.payload->buf.buf_len = strlen(stringa) + 1;
		bundle.payload->buf.buf_val = malloc(sizeof(char) * bundle.payload->buf.buf_len);

		strcpy(bundle.payload->buf.buf_val, stringa);

		printf("%d\n", al_bp_check_bp_options(argc, argv, &bundle_options, &result_options));

		printf("%d\n", al_bp_create_bundle_with_option(&bundle, bundle_options));

		printf("%d\n", al_bp_extB_send(rd, bundle, dest, none));
		
		printf("Sent bundle to %s: %s\n", dest.uri, bundle.payload->buf.buf_val);
		
		al_bp_bundle_free(&bundle);
		al_bp_bundle_create(&bundle);
		
		printf("%d\n", al_bp_extB_receive(rd, &bundle, BP_PAYLOAD_MEM, 20));
	
		printf("Received by %s: %s\n", bundle.spec->source.uri, bundle.payload->buf.buf_val);

		// This function deallocate memory allocated with bp_bundle_create()
		al_bp_bundle_free(&bundle);
		al_bp_free_result_options(result_options);
	}
	
	// This function unregisters the registration identified by the registration descriptor.
	al_bp_extB_unregister(rd);
	
	// This function destroys the registration list.
	al_bp_extB_destroy();
	return 0;
}


