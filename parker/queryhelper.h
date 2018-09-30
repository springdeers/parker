#ifndef __QUERYHELPER_H__
#define __QUERYHELPER_H__

struct cJSON;
struct evkeyvalq;

int sqlpartstr_fromjson(struct cJSON* obj, char*fields[], int fieldcount, char* name, char* obuffer, int obufferlen);
int sqlpartstr_fromquery(struct evkeyvalq*kvq, char*fields[], int fieldcount, char* name, char* obuffer, int obufferlen);
int http_response_wrong(struct evkeyvalq*kvq, struct evhttp_request* req, void* param, char* strerr);
#endif