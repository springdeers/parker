#include "orderquery.h"
#include "route.h"
#include <memory.h>
#include <stdio.h>
#include "event2/event.h"
#include "event2/keyvalq_struct.h"
#include "event2/buffer.h"
#include "event2/http.h"
#include "event2/http_struct.h"
#include "cutil/util.h"
#include "membuff.h"
#include "mysqldb.h"

extern mysqlquery_t mysqlconn;
static membuff_t g_membuffer = NULL;
extern log_t  g_log;

#define NULL 0

static char* g_helpstr = NULL;

static int get_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int add_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int del_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int mod_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);

static int get_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int add_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int del_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int mod_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);

static int route_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);

static route_entry_st route_map[] = {
	{ "/order/unsettled/get", get_unsettled_order,"查询未处理的订单"},
	{ "/order/unsettled/add", add_unsettled_order,"添加未处理的订单"},
	{ "/order/unsettled/del", del_unsettled_order,"删除未处理的订单"},
	{ "/order/unsettled/mod", mod_unsettled_order,"更改未处理的订单"},

	{ "/order/settled/get", get_settled_order, "查询已处理的订单"},
	{ "/order/settled/add", add_settled_order, "添加已处理的订单"},
	{ "/order/settled/del", del_settled_order, "删除已处理的订单"},
	{ "/order/settled/mod", mod_settled_order, "更改已处理的订单"},
	{ "/order/unsettled/help", route_help,"帮助" },
	{ "/order/settled/help", route_help,"帮助" },
	{ NULL, NULL },
};

int route_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	struct evbuffer* buf = evbuffer_new();

	if (g_helpstr == NULL){
		FILE* file = fopen("./wapihelp.txt", "rb");
		if (file){
			int len = 0;
			fseek(file, 0, SEEK_END);
			len = ftell(file);
			fseek(file, 0, 0);

			g_helpstr = (char*)calloc(1, len + 1);

			fread(g_helpstr, 1, len, file);
			fclose(file);
		}

		if (g_helpstr == NULL)
			g_helpstr = "help file not exist.";
	}

	if (!buf || req == NULL) return eRoute_failed;

	if (g_membuffer == NULL) g_membuffer = membuff_new(4096);
	membuff_clear(g_membuffer);

	membuff_add_printf(g_membuffer, "{\"type\":\"help\",\"code\":\"-1\",\"rslt\":");
	membuff_add_printf(g_membuffer, "\"%s\"", g_helpstr);
	membuff_addchar(g_membuffer, '}');
	membuff_addchar(g_membuffer, '\0');

	evbuffer_add_printf(buf, g_membuffer->data);
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);

	return eRoute_success;
}

orderquery_t orderquery_new()
{
	orderquery_t rt = calloc(1, sizeof(orderquery_st));
	rt->_base.name = "order";
	rt->_base.entries = route_map;
	rt->_base.entrycount = sizeof(route_map) / sizeof(route_map[0]);

	rt->_base._help = route_help;
	rt->_base._free = orderquery_free;

	return rt;
}

void   orderquery_free(orderquery_t query)
{
	if (query) free(query);
}

static int get_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	printf("%s","get_unsettled_order");
	return eRoute_success;
}

static int add_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	printf("%s", "add_unsettled_order");
	return eRoute_success;
}

static int del_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	printf("%s", "del_unsettled_order");
	return eRoute_success;
}

static int mod_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	printf("%s", "mod_unsettled_order");
	return eRoute_success;
}

static int get_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	printf("%s", "get_settled_order");
	return eRoute_success;
}

static int add_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	printf("%s", "add_settled_order");
	return eRoute_success;
}

static int del_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	printf("%s", "del_settled_order");
	return eRoute_success;
}
static int mod_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	printf("%s", "mod_settled_order");
	return eRoute_success;
}