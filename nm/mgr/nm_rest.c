
// System includes
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

// CivetWeb includes
#include "cJSON.h"
#include "civetweb.h"
#include "nm_rest.h"

// NM Includes
#include "../shared/nm.h"
#include "agents.h"
#include "nm_mgr.h"
#include "nm_mgr_ui.h"
#include "nm_mgr_print.h"

#include "ion.h" // Used only to retrieve IONVERSIONNUMBER for reference

struct mg_context *ctx;

#define MAX_INPUT_BYTES 4096

#define BASE_API_URI "/nm/api"
#define VERSION_URI BASE_API_URI "/version"
#define AGENTS_URI BASE_API_URI "/agents"
#define AGENTS_IDX_URI AGENTS_URI "/idx"
#define AGENTS_EID_URI AGENTS_URI "/eid"

// REST API Handler Declarations
static int versionHandler(struct mg_connection *conn, void *cbdata);
static int agentsHandler(struct mg_connection *conn, void *cbdata);
static int agentIdxHandler(struct mg_connection *conn, void *cbdata);
static int agentEidHandler(struct mg_connection *conn, void *cbdata);

/*** Start Common API Functions ***/
static int
SendJSON(struct mg_connection *conn, cJSON *json_obj)
{
	char *json_str = cJSON_PrintUnformatted(json_obj);
	size_t json_str_len = strlen(json_str);

	/* Send HTTP message header */
	mg_send_http_ok(conn, "application/json; charset=utf-8", json_str_len);

	/* Send HTTP message content */
	mg_write(conn, json_str, json_str_len);

	/* Free string allocated by cJSON_Print* */
	cJSON_free(json_str);

	return (int)json_str_len;
}

static void start_text_page(struct mg_connection *conn, char* msg, ...)
{
   va_list args;
   va_start(args, msg);
   mg_printf(conn,
             "HTTP/1.1 200 OK\r\nContent-Type: "
             "text/plain\r\nConnection: close\r\n\r\n");
   mg_vprintf(conn, msg, args);
   va_end(args);
}


int
log_message(const struct mg_connection *conn, const char *message)
{
   // TODO: Wrap with AMP macro for logging
	puts(message);
	return 1;
}

int nm_rest_start()
{
	const char *options[] = {"listening_ports",
	                         PORT,
	                         "request_timeout_ms",
	                         "10000",
	                         "error_log_file",
	                         "error.log",
#ifndef NO_SSL
	                         "ssl_certificate",
	                         "../../resources/cert/server.pem",
	                         "ssl_protocol_version",
	                         "3",
	                         "ssl_cipher_list",
	                         "DES-CBC3-SHA:AES128-SHA:AES128-GCM-SHA256",
#endif
	                         "enable_auth_domain_check",
	                         "no",
	                         0};

   struct mg_callbacks callbacks;
	int err = 0;

/* Check if libcivetweb has been built with all required features. */
#ifndef NO_SSL
	if (!mg_check_feature(2)) {
		fprintf(stderr,
		        "Error: Embedded example built with SSL support, "
		        "but civetweb library build without.\n");
		err = 1;
	}


	mg_init_library(MG_FEATURES_SSL);

#else
	mg_init_library(0);

#endif
	if (err) {
		fprintf(stderr, "Cannot start CivetWeb - inconsistent build.\n");
		return EXIT_FAILURE;
	}


	/* Callback will print error messages to console */
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.log_message = log_message;

	/* Start CivetWeb web server */
	ctx = mg_start(&callbacks, 0, options);

	/* Check return value: */
	if (ctx == NULL) {
		fprintf(stderr, "Cannot start CivetWeb - mg_start failed.\n");
		return EXIT_FAILURE;
	}

	/* Add URL Handlers.   */
	mg_set_request_handler(ctx, VERSION_URI, versionHandler, 0);

    mg_set_request_handler(ctx, AGENTS_IDX_URI, agentIdxHandler, 0);
    mg_set_request_handler(ctx, AGENTS_EID_URI, agentEidHandler, 0);
    mg_set_request_handler(ctx, AGENTS_URI, agentsHandler, 0);
    printf("REST API Server Started on port " PORT "\n");
    return EXIT_SUCCESS;
}

void nm_rest_stop()
{
   if (ctx != NULL) {
      mg_stop(ctx);
   }
}


/*** Start REST API Handlers ***/

static int versionHandler(struct mg_connection *conn, void *cbdata)
{
   cJSON *obj;
   const struct mg_request_info *ri = mg_get_request_info(conn);
   (void)cbdata; /* currently unused */

   if (0 != strcmp(ri->request_method, "GET")) {
      	mg_send_http_error(conn,
                           405,
                           "Only GET method supported for this page");
        return 405;
   }
   
   obj = cJSON_CreateObject();

	if (!obj) {
		/* insufficient memory? */
		mg_send_http_error(conn, 500, "Server error");
		return 500;
	}


	cJSON_AddStringToObject(obj, "civetweb_version", CIVETWEB_VERSION);
	cJSON_AddNumberToObject(obj, "amp_version", AMP_VERSION);
    cJSON_AddStringToObject(obj, "amp_uri", AMP_PROTOCOL_URL);
	cJSON_AddStringToObject(obj, "amp_version_str", AMP_VERSION_STR);
    
    cJSON_AddStringToObject(obj, "bp_version",
#ifdef BUILD_BPv6
                            "6"
#elif defined(BUILD_BPv7)
                            "7"
#else
                            "?"
#endif
       );
                            
    cJSON_AddStringToObject(obj, "build_date", __DATE__);
    cJSON_AddStringToObject(obj, "build_time", __TIME__);
	SendJSON(conn, obj);
	cJSON_Delete(obj);

	return 200;
}

static int agentsGETHandler(struct mg_connection *conn)
{
   cJSON *obj = cJSON_CreateObject();
   cJSON *agentList = NULL;
   cJSON *agentObj;
   vecit_t it;
   agent_t * agent = NULL;
   
   if (!obj) {
      /* insufficient memory? */
      mg_send_http_error(conn, 500, "Server error");
      return 500;
   }

   agentList = cJSON_AddArrayToObject(obj, "agents");
   if (agentList == NULL)
   {
      cJSON_Delete(obj);
      mg_send_http_error(conn, 500, "Server error (can't allocate array for agents)");
      return 500;
   }


  for(it = vecit_first(&(gMgrDB.agents)); vecit_valid(it); it = vecit_next(it))
  {
     agentObj = cJSON_CreateObject();
     agent = (agent_t *) vecit_data(it);

     cJSON_AddStringToObject(agentObj, "name", agent->eid.name);
     cJSON_AddNumberToObject(agentObj, "rpts_count", vec_num_entries(agent->rpts) );
     cJSON_AddNumberToObject(agentObj, "tbls_count", vec_num_entries(agent->tbls) );
     cJSON_AddItemToArray(agentList, agentObj);
  }

  SendJSON(conn, obj);
  cJSON_Delete(obj);

  return HTTP_OK;
   
}

// This may be called via POST /agents or PUT /agents/$name
static int agentsCreateHandler(struct mg_connection *conn, char *name)
{
   eid_t agent_eid;

   // Sanity-check string length
   if (name == NULL || strlen(name) > AMP_MAX_EID_LEN-2)
   {
      mg_send_http_error(conn, 400, "Invalid EID length");
      return 400;
   }
   else
   {
      strcpy(agent_eid.name, name);
      if (agent_add(agent_eid) == AMP_OK)
      {
         mg_send_http_error(conn, 400, "Unable to register agent");
         return 400;
      }
      else
      {
         return HTTP_NO_CONTENT;
      }
   }
}

static int agentsHandler(struct mg_connection *conn, void *cbdata)
{
   const struct mg_request_info *ri = mg_get_request_info(conn);
   (void)cbdata; /* currently unused */ 

   if (0 == strcmp(ri->request_method, "GET")) {
      return agentsGETHandler(conn);
   } else if (0 == strcmp(ri->request_method, "POST")) {
      char buffer[AMP_MAX_EID_LEN+1];
      int dlen = mg_read(conn, buffer, sizeof(buffer));
      if ( dlen < 1 ) {
         mg_send_http_error(conn, 400, "Invalid request body data (expect EID name) %d", dlen);
         return 400;
      } else {
         return agentsCreateHandler(conn, buffer);
      }
   } else {
      	mg_send_http_error(conn,
                           405,
                           "Only GET and PUT methods supported for this page");
        return 405;
   }

}

static int agentSendRaw(struct mg_connection *conn, time_t ts, agent_t *agent, char *hex)
{
   ari_t *id = NULL;
   msg_ctrl_t *msg = NULL;
   int success;

   blob_t *data = utils_string_to_hex(hex);
   if (data == NULL) {
      mg_send_http_error(conn,
                         500,
                         "Error creating blob from input");
      return 500;
   }
   id = ari_deserialize_raw(data, &success);
   blob_release(data, 1);
   if (id == NULL) {
      mg_send_http_error(conn,
                         500,
                         "Error creating blob from input");
      return 500;
   }

   
   ui_postprocess_ctrl(id);

   if((msg = msg_ctrl_create_ari(id)) == NULL)
   {
      ari_release(id, 1);
      mg_send_http_error(conn,
                         500,
                         "Error creating ARI from input");
      return HTTP_INTERNAL_ERROR;
   }
   msg->start = ts;
   iif_send_msg(&ion_ptr, MSG_TYPE_PERF_CTRL, msg, agent->eid.name);
   ui_log_transmit_msg(agent, msg);
   msg_ctrl_release(msg, 1);

   start_text_page(conn, "Successfully sent Raw ARI Control");
   return HTTP_OK;
}

static int agentShowTextReports(struct mg_connection *conn, agent_t *agent)
{
   vecit_t rpt_it;
   ui_print_cfg_t fd = INIT_UI_PRINT_CFG_CONN(conn);

   if (agent == NULL)
   {
      return HTTP_INTERNAL_ERROR;
   }

   start_text_page(conn,
                   "Showing %d reports for agent %s",
                   vec_num_entries_ptr(&(agent->rpts)),
                   agent->eid.name);

   /* Iterate through all reports for this agent. */
   for(rpt_it = vecit_first(&(agent->rpts)); vecit_valid(rpt_it); rpt_it = vecit_next(rpt_it))
   {
      ui_fprint_report(&fd, (rpt_t*)vecit_data(rpt_it));
   }
   
   return HTTP_OK;

}

static int agentShowJSONReports(struct mg_connection *conn, agent_t *agent)
{
   cJSON *obj;
   cJSON *reports;
   vecit_t rpt_it;
   ui_print_cfg_t fd = INIT_UI_PRINT_CFG_CONN(conn);

   if (agent == NULL)
   {
      return HTTP_INTERNAL_ERROR;
   }

   obj = cJSON_CreateObject();
   cJSON_AddStringToObject(obj, "eid", agent->eid.name);
   reports = cJSON_AddArrayToObject(obj, "reports");
   
   /* Iterate through all reports for this agent. */
   for(rpt_it = vecit_first(&(agent->rpts)); vecit_valid(rpt_it); rpt_it = vecit_next(rpt_it))
   {
      rpt_t *rpt = ( (rpt_t*)vecit_data(rpt_it) );

      cJSON_AddItemToArray(reports, ui_json_report(rpt) );
   }

   SendJSON(conn, obj);
   cJSON_Delete(obj);
   return HTTP_OK;

}

static int agentShowRawReports(struct mg_connection *conn, agent_t *agent)
{
   cJSON *obj;
   cJSON *reports;
   vecit_t rpt_it;

   if (agent == NULL)
   {
      return HTTP_INTERNAL_ERROR;
   }
   
   obj = cJSON_CreateObject();
   cJSON_AddStringToObject(obj, "eid", agent->eid.name);
   reports = cJSON_AddArrayToObject(obj, "reports");
   
   /* Iterate through all reports for this agent. */
   for(rpt_it = vecit_first(&(agent->rpts)); vecit_valid(rpt_it); rpt_it = vecit_next(rpt_it))
   {
      // TODO: Erorr checking
      // TODO: Prepend "rpt:" to string to match amp.me convention for decoding
      
      blob_t *rpt = rpt_serialize_wrapper( (rpt_t*)vecit_data(rpt_it) );
      char *rpt_str = utils_hex_to_string(rpt->value, rpt->length);

      cJSON_AddItemToArray(reports, cJSON_CreateStringReference( rpt_str ));
      SRELEASE(rpt_str);
      blob_release(rpt,1);
   }

   SendJSON(conn, obj);
   cJSON_Delete(obj);
   return HTTP_OK;

}

/** Handler for /agents/eid*
 *    Supported requests:
 *    - PUT /agents/eid/$eid/hex - Send HEX-encoded CBOR Command (hex string as request body).
 *    - PUT /agents/eid/$eid/hex/$ts - Same as above, but with Timestamp value given in URL, whereas above assumes a value of 0.
 *    - PUT /agents/eid/$eid/clear_reports - Clear all received reports for this agent.
 *    - PUT /agents/eid/$eid/clear_tables - Clear all received tables for this agent.
 *    - GET /agents/eid/$eid/reports/hex - Retrieve array of reports in CBOR-encoded HEX form
 *    - GET /agents/eid/$eid/reports/text - Retrieve array of reports in ASCII Text form (same as ui)
 *    - GET /agents/eid/$eid/reports* - Alias for hex reports. format will change in the future.
 */
static int agentEidHandler(struct mg_connection *conn, void *cbdata)
{
   const struct mg_request_info *ri = mg_get_request_info(conn);
   (void)cbdata; /* currently unused */

   char eid[32] = "";
   char cmd[32] = "";
   char cmd2[32] = "";
   
   int cnt = sscanf(ri->local_uri, AGENTS_URI "/eid/%[^/?]/%[^/?]/%[^/?]", eid, cmd, cmd2);
   
   if (cnt >= 2 && 0 == strcmp(ri->request_method, "PUT") )
   {
      agent_t *agent = agent_get((eid_t*)eid);

      if ( agent == NULL)
      {
         mg_send_http_error(conn,
                            HTTP_NOT_FOUND,
                            "Invalid agent eid");
         return HTTP_NOT_FOUND;
      }
      else if (0 == strcmp(cmd, "hex"))
      {
         // URL idx field translates to agent name
         // Optional query parameter "ts" will translate to timestamp (TODO: Always 0 for initial cut)
         // Request body contains CBOR-encoded HEX string
         char buffer[MAX_INPUT_BYTES];
         int dlen = mg_read(conn, buffer, sizeof(buffer));
         if (dlen <= 0) {
            return HTTP_BAD_REQUEST;
         }
         buffer[dlen] = 0; // Ensure string is NULL-terminated
         int ts = 0;
         if (cnt == 3) {
            // Optional Timestamp as last element of URL path
            ts = atoi(cmd2);
         }
         return agentSendRaw(conn,
                            ts,
                            agent,
                            buffer
         );
      }
      else if (0 == strcmp(cmd, "clear_reports"))
      {
         ui_clear_reports(agent);
         start_text_page(conn, "Successfully cleared reports");
         return HTTP_OK;
      }
      else if (0 == strcmp(cmd, "clear_tables"))
      {
         ui_clear_tables(agent);
         start_text_page(conn, "Successfully cleared tables");
         return HTTP_OK;
      }
   }
   else if (cnt >= 2 && 0 == strcmp(ri->request_method, "GET"))
   {
      if (0 == strcmp(cmd, "reports"))
      {
         if (cnt == 2 || 0 == strcmp(cmd2, "text") )
         {
            return agentShowTextReports(conn,agent_get((eid_t*)eid) );
         }
         else if (cnt == 3 && 0 == strcmp(cmd2, "hex") )
         {
            return agentShowRawReports(conn, agent_get((eid_t*)eid) );
         }
         else if (cnt == 3 && 0 == strcmp(cmd2, "json") )
         {
            return agentShowJSONReports(conn, agent_get((eid_t*)eid) );
         }
      }
    }

   // Invalid request if we make it to this point     
   mg_send_http_error(conn,
                      HTTP_METHOD_NOT_ALLOWED,
                      "Invalid request.");
   return HTTP_METHOD_NOT_ALLOWED;
   
}

/** Handler for /agents/idx*
 *    Supported requests:
 *    - PUT /agents/idx/$idx/hex - Send HEX-encoded CBOR Command (hex string as request body)
 *    - PUT /agents/idx/$idx/clear_reports - Clear all received reports for this agent.
 *    - PUT /agents/idx/$idx/clear_tables - Clear all received tables for this agent.
 *    - GET /agents/idx/$idx/reports/hex - Retrieve array of reports in CBOR-encoded HEX form
 *    - GET /agents/idx/$idx/reports/text - Retrieve array of reports in ASCII Text form (same as ui)
 *    - GET /agents/idx/$idx/reports* - Alias for hex reports. format will change in the future.
 */
static int agentIdxHandler(struct mg_connection *conn, void *cbdata)
{
   const struct mg_request_info *ri = mg_get_request_info(conn);
   (void)cbdata; /* currently unused */

   uint32_t idx = 0;
   char cmd[32] = "";
   char cmd2[32] = "";
   
   int cnt = sscanf(ri->local_uri, AGENTS_URI "/idx/%d/%[^/?]/%[^/?]", &idx, cmd, cmd2);
   
   if (cnt == 2 && 0 == strcmp(ri->request_method, "PUT") )
   {
      agent_t *agent = (agent_t*) vec_at(&gMgrDB.agents, idx);

      if ( idx >= vec_num_entries_ptr(&gMgrDB.agents) || agent == NULL)
      {
         mg_send_http_error(conn,
                            HTTP_NOT_FOUND,
                            "Invalid agent idx");
         return HTTP_NOT_FOUND;
      }
      else if (0 == strcmp(cmd, "hex"))
      {
         // URL idx field translates to agent name
         // Optional query parameter "ts" will translate to timestamp (TODO: Always 0 for initial cut)
         // Request body contains CBOR-encoded HEX string
         char buffer[MAX_INPUT_BYTES];
         int dlen = mg_read(conn, buffer, sizeof(buffer));
         return agentSendRaw(conn,
                            0, // Timestamp TODO. This will be an optional query param
                            agent,
                            buffer
         );
      }
      else if (0 == strcmp(cmd, "clear_reports"))
      {
         ui_clear_reports(agent);
         start_text_page(conn, "Successfully cleared reports");
         return HTTP_OK;
      }
      else if (0 == strcmp(cmd, "clear_tables"))
      {
         ui_clear_tables(agent);
         start_text_page(conn, "Successfully cleared tables");
         return HTTP_OK;
      }
   }
   else if (cnt >= 2 && 0 == strcmp(ri->request_method, "GET"))
   {
      if (0 == strcmp(cmd, "reports"))
      {
         if (cnt == 2 || 0 == strcmp(cmd2, "text") )
         {
            return agentShowTextReports(conn,
                                      (agent_t*) vec_at(&gMgrDB.agents, idx)
            );

         }
         else if (cnt == 3 && 0 == strcmp(cmd2, "hex") )
         {
            return agentShowRawReports(conn,
                                      (agent_t*) vec_at(&gMgrDB.agents, idx)
            );
         }
         else if (cnt == 3 && 0 == strcmp(cmd2, "json") )
         {
            return agentShowJSONReports(conn,
                                      (agent_t*) vec_at(&gMgrDB.agents, idx)
            );
         }
      }
    }

   // Invalid request if we make it to this point     
   mg_send_http_error(conn,
                      HTTP_METHOD_NOT_ALLOWED,
                      "Invalid request.");
   return HTTP_METHOD_NOT_ALLOWED;

}
