#include "queryhelper.h"
#include <time.h>
#include "cJSON.h"
#include "event2/event.h"
#include "event2/keyvalq_struct.h"
#include "event2/buffer.h"
#include "event2/http.h"
#include "event2/http_struct.h"
#include "route.h"
#include "membuff.h"

#define NULL 0

int  sqlpartstr_fromjson(struct cJSON* obj, char*fields[], int fieldcount, char* name, char* obuffer, int obufferlen)
{
	int printn = 0;
	if (name == NULL || obj == NULL || obuffer == NULL || obufferlen < 1) return 0;

	if (strcmp(name, "where") == 0){
		for (int i = 0; i < fieldcount; i++){
			cJSON* item = cJSON_GetObjectItem(obj, fields[i]);
			if (item && item->valuestring && strlen(item->valuestring)){
				printn += sprintf_s(obuffer + printn, obufferlen - 1 - printn, "%s=%s and ", fields[i], item->valuestring);
			}
		}
		char* palpha_a = strrchr(obuffer, 'a');
		*palpha_a = '\0';

		return 1;
	}
	else if (strcmp(name, "set") == 0){
		for (int i = 0; i < fieldcount; i++){
			cJSON* item = cJSON_GetObjectItem(obj, fields[i]);
			if (item && item->valuestring && strlen(item->valuestring)){
				printn += sprintf_s(obuffer + printn, obufferlen - 1 - printn, "%s=%s,", fields[i], item->valuestring);
			}
		}
		char* palpha_comma = strchr(obuffer, ',');
		*palpha_comma = '\0';

		return 1;
	}

	return 0;
}

int sqlpartstr_fromquery(struct evkeyvalq*kvq, char*fields[], int fieldcount, char* name, char* obuffer, int obufferlen)
{
	int printn = 0;

	for (int i = 0; i < fieldcount; i++){
		const char* val = evhttp_find_header(kvq, fields[i]);
		if (val && strlen(val) > 0){
			printn += sprintf_s(obuffer + printn, obufferlen - 1 - printn, "%s=%s and ", fields[i], val);
		}
	}

	char * alpha_a = strrchr(obuffer, 'a');
	*alpha_a = '\0';

	return 1;
}

int http_response_wrong(struct evkeyvalq*kvq, struct evhttp_request* req, void* param,char* strerr)
{
	membuff_t mb = membuff_new(1024);

	struct evbuffer* buf = NULL;
	
	while ((buf = evbuffer_new()) == NULL) Sleep(1);

	if (req == NULL) return eRoute_failed;

	if (strerr == NULL) strerr = "";
	membuff_add_printf(mb, "{\"rslt\":\"failed\",\"reason\":\"%s\"}", strerr);

	evbuffer_add_printf(buf, mb->data);
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	//clear jobs..
	membuff_free(mb);
	evbuffer_free(buf);

	return eRoute_success;
}