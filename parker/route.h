#ifndef _ROUTE_H__
#define _ROUTE_H__

enum{eRoute_null,eRoute_success,eRoute_failed,eRoute_wrongparam};

typedef int (*route_pfn)(struct evkeyvalq*kvq,struct evhttp_request* req,void* param);

typedef struct _route_entry_st{
	char* _path;
	route_pfn  _route;
	char* desc;
}route_entry_st,*route_entry_t;

typedef int (*router_help_t)(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
typedef void(*router_free_t)();
typedef struct _router_st{
	char* name;
	route_entry_t entries;
	int   entrycount;
	struct _router_st* next;
	int router_size;
	router_free_t _free;
	router_help_t _help;
}router_st,*router_t;

router_t   router_setup();
router_t   router_add(router_t router, router_t tail);

void       router_free(router_t router);
int        router_help(router_t router, struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
router_t   router_object();

int route(router_t router,char* path, struct evkeyvalq*kvq, struct evhttp_request* req, void* param);

#endif