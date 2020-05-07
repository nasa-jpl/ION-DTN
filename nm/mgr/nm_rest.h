#ifndef __NM_REST_H__
#define __NM_REST_H__

// TODO: Allow these to be configurable
#ifdef NO_SSL
#define PORT "8089"
#define HOST_INFO "http://localhost:8089"
#else
#define PORT "8089r,8843s"
#define HOST_INFO "https://localhost:8843"
#endif

int nm_rest_start();
void nm_rest_stop();

// Standard HTTP Status Codes
#define HTTP_OK                  200
#define HTTP_NO_CONTENT          204
#define HTTP_BAD_REQUEST         400
#define HTTP_FORBIDDEN           403
#define HTTP_NOT_FOUND           404
#define HTTP_METHOD_NOT_ALLOWED  405
#define HTTP_INTERNAL_ERROR      500
#define HTTP_NOT_IMPLEMENTED     501
#define HTTP_NO_SERVICE          503


#endif
