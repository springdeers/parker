#include "route.h"
#include "dbaccess.h"
#include "event2/event.h"
#include "event2/keyvalq_struct.h"
#include "event2/buffer.h"
#include "event2/http.h"
#include "event2/http_struct.h"
#include "cutil/util.h"
#include "membuff.h"
#include "visitrslt.h"
#include "orderquery.h"
#include "userquery.h"

extern log_t  g_log;

static router_t      g_router;

static char* g_helpstr = NULL;

router_t   router_setup()
{
	g_router = userquery_new();
	g_router->router_size = 1;
	router_add(g_router, orderquery_new());
	//router_add(g_router, cardquery_new());

	return g_router;
}

router_t   router_object()
{
	return g_router;
}

int router_help(router_t router, struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	if (router->_help) return router->_help(kvq, req, param);	
	return eRoute_failed;
}

router_t   router_add(router_t dest, router_t tail)
{
	int size = 0;
	if (dest == NULL || tail == NULL) return NULL;

	size = dest->router_size;

	(dest+ size - 1)->next = tail;
	dest->router_size++;

	return dest;
}

void    router_free(router_t router)
{
	router_t curr = router;	

	while (curr){
		router_t next = curr->next;
		curr->_free(curr);
		curr = next;
	}
}

int route(router_t router, char* path, struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int rtn = eRoute_null;
	int i   = 0;

	while (router != NULL){
		for (; i < router->entrycount; i++){
			if (router->entries[i]._path != NULL && strcmp(path, router->entries[i]._path) == 0){
				rtn = router->entries[i]._route(kvq, req, param);
				log_write(g_log, LOG_NOTICE, "serve path %s code[%d] %s", router->entries[i]._path, rtn, rtn == 0 ? "success" : "failed");
				break;
			}
		}

		if (rtn == eRoute_success){
			break;
		}else if (rtn == eRoute_failed){
			rtn = router_help(router, kvq, req, param);
			break;
		}
		
		router = router->next;
	}
	
	return rtn;
}

