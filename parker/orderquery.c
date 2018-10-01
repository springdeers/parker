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
#include "cJSON.h"
#include "tool.h"
#include "dbaccess.h"
#include "httpquery_helper.h"

extern mysqlquery_t sqlobj_venue_db;
static membuff_t g_membuffer = NULL;
extern log_t  g_log;

#define NULL 0

static char* g_helpstr = NULL;
static char* g_fields_unsettled[] = { _field_orderno, _field_orderdate, _field_username, _field_age, _field_cardid, _field_telephone, _field_workunit, _field_email, _field_team, _field_group, _field_teamleader };
static char* g_fields_settled[]   = { _field_orderno, _field_orderdate, _field_username, _field_age, _field_cardid, _field_telephone, _field_workunit, _field_email, _field_team, _field_group, _field_teamleader, _field_operator, _field_opertime, _field_price };

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

	if (req == NULL) return http_response_error(kvq, req, param,"请求错误.");

	_assure_clearbuff(g_membuffer, 4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"help\",\"code\":\"-1\",\"rslt\":");
	membuff_add_printf(g_membuffer, "\"%s\"", g_helpstr);
	membuff_addchar(g_membuffer, '}');
	membuff_addchar(g_membuffer, '\0');

	http_response_ok(kvq, req, param, g_membuffer->data);

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

static int _jparse_unsettledorder(cJSON* jorder, unsettled_order_t order)
{
	cJSON * obj = NULL;

	if (jorder == NULL || order == NULL) return 0;

	if (obj = cJSON_GetObjectItem(jorder, _field_orderdate))
		strcpy_s(order->orderdate, sizeof(order->orderdate) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_username))
		strcpy_s(order->username, sizeof(order->username) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_age))
		order->age = s_atoi(obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_cardid))
		strcpy_s(order->cardid, sizeof(order->cardid) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_telephone))
		strcpy_s(order->telephone, sizeof(order->telephone) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_workunit))
		strcpy_s(order->workunit, sizeof(order->workunit) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_email))
		strcpy_s(order->email, sizeof(order->email) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_team))
		strcpy_s(order->team, sizeof(order->team) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_group))
		strcpy_s(order->group, sizeof(order->group) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_teamleader))
		strcpy_s(order->teamleader, sizeof(order->teamleader) - 1, obj->valuestring);

	return 1;
}

static int _jparse_settledorder(cJSON* jorder, settled_order_t order)
{
	cJSON * obj = NULL;

	if (jorder == NULL || order == NULL) return 0;

	if (obj = cJSON_GetObjectItem(jorder, _field_orderdate))
		strcpy_s(order->orderdate, sizeof(order->orderdate) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_username))
		strcpy_s(order->username, sizeof(order->username) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_age))
		order->age = s_atoi(obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_cardid))
		strcpy_s(order->cardid, sizeof(order->cardid) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_telephone))
		strcpy_s(order->telephone, sizeof(order->telephone) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_workunit))
		strcpy_s(order->workunit, sizeof(order->workunit) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_email))
		strcpy_s(order->email, sizeof(order->email) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_team))
		strcpy_s(order->team, sizeof(order->team) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_group))
		strcpy_s(order->group, sizeof(order->group) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_teamleader))
		strcpy_s(order->teamleader, sizeof(order->teamleader) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_operator))
		strcpy_s(order->operator, sizeof(order->operator) - 1, obj->valuestring);

	if (obj = cJSON_GetObjectItem(jorder, _field_opertime))
		strcpy_s(order->opertime, sizeof(order->opertime) - 1, obj->valuestring);

	return 1;
}

static int get_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int  num = 0;
	char where[256] = {0};
	int  printn = 0;
	unsettled_order_t * orders = NULL;
	int  count = 0;
	
	sqlpartstr_fromquery(kvq, g_fields_unsettled, _arraysize(g_fields_unsettled), "where", where, sizeof(where));

	int query = db_query_unsettled_orders(sqlobj_venue_db, where, &orders, &count);

	_assure_clearbuff(g_membuffer, 64 * 1024);

	if (query){
 		membuff_add_printf(g_membuffer, "{\"rslt\":\"ok\",\"data\":[");
		for (int i = 0; i < count; i++){
			membuff_add_printf(g_membuffer, "{\"orderdate\":\"%s\",\"orderno\":\"%s\",\"username\":\"%s\",\"age\":\"%d\",\"cardid\":\"%s\",\"telephone\":\"%s\",\"workunit\":\"%s\",\
											  \"email\":\"%s\",\"team\":\"%s\",\"group\":\"%s\",\"teamleader\":\"%s\"},",
											  orders[i]->orderdate,
											  orders[i]->orderno,
											  orders[i]->username,
											  orders[i]->age,
											  orders[i]->cardid,
											  orders[i]->telephone,
											  orders[i]->workunit,
											  orders[i]->email,
											  orders[i]->team,
											  orders[i]->group,
											  orders[i]->teamleader);
		}
		membuff_trim(g_membuffer,",");
		membuff_addchar(g_membuffer, ']');
		membuff_addchar(g_membuffer, '\0');
	}
	else{
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");
	}

	http_response_ok(kvq, req, param, g_membuffer->data);

	//clear job..
	free(orders);

	return eRoute_success;
}

static int add_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int    saved    = 0;
	cJSON *jroot = NULL;
	int    ordernum = 0;
	int    evbuflen = 0;
	unsettled_order_t orders  = NULL;
	char  *reqbody = http_request_body(req);

	if (reqbody == NULL || strlen(reqbody) == 0 || (jroot = cJSON_Parse((const char*)reqbody)) == NULL){
		if (reqbody) free(reqbody);
		return  http_response_error(kvq, req, param, "参数错误.");
	}

	if ((ordernum = cJSON_GetArraySize(jroot)) == 0)
	{
		if (reqbody) free(reqbody);
		return  http_response_error(kvq, req, param, "参数错误.");
	}

	while ((orders = calloc(ordernum, sizeof(unsettled_order_st))) == NULL) Sleep(1);

	cJSON* obj = NULL;
	for (int i = 0; i < ordernum; i++)
	{
		obj = cJSON_GetArrayItem(jroot, i);

		if (obj == NULL || !_jparse_unsettledorder(obj, &orders[i]))
		{
			cJSON_Delete(jroot);
			free(orders);
			free(reqbody);
			return  http_response_error(kvq, req, param, "参数错误.");
		}
	}

	saved = db_save_unsettled_orders(sqlobj_venue_db, orders, ordernum);

	_assure_clearbuff(g_membuffer, 64 * 1024);

	if (saved)
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");

	http_response_ok(kvq, req, param, g_membuffer->data);

	//clear jobs..
	cJSON_Delete(jroot);
	free(orders);
	free(reqbody);

	printf("%s", "add_unsettled_order.\n");

	return eRoute_success;
}

static int del_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int deleted = 0;
	const char* orderno  = evhttp_find_header(kvq, _field_orderno);
	const char* username = evhttp_find_header(kvq, _field_username);

	if (orderno == NULL) return  http_response_error(kvq, req, param, "参数错误.");;

	//////////////////////////////////////////////////////////////////////////
	deleted = db_delete_unsettled_orders(sqlobj_venue_db, orderno, username);
	_assure_clearbuff(g_membuffer, 64*1024);

	if (deleted)
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else{
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");
	}

	http_response_ok(kvq, req, param, g_membuffer->data);
	
	printf("%s", "del_unsettled_order");
	return eRoute_success;
}

static int mod_unsettled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int    modified = 0;
	int    evbuflen = 0;
	cJSON* jroot    = NULL;
	char   where[256] = {0};
	char   set[256]   = {0};
	int    formed = 0;
	char*  reqbody = http_request_body(req);

	if (reqbody == NULL || strlen(reqbody) == 0 || (jroot = cJSON_Parse((const char*)reqbody)) == NULL) {
		if (reqbody) free(reqbody);
		return  http_response_error(kvq, req, param, "参数错误.");
	}

	cJSON* jwhere = cJSON_GetObjectItem(jroot,"where");
	cJSON* jset   = cJSON_GetObjectItem(jroot, "set");

	if (jset == NULL || jwhere == NULL){
		free(reqbody); cJSON_Delete(jroot); 
		return http_response_error(kvq, req, param, "参数错误.");;
	}
	
	if (!sqlpartstr_fromjson(jwhere, g_fields_unsettled, _arraysize(g_fields_unsettled), "where", where, sizeof(where))||
		!sqlpartstr_fromjson(jwhere, g_fields_unsettled, _arraysize(g_fields_unsettled), "set", set, sizeof(set)))
	{ 
		free(reqbody); cJSON_Delete(jroot); 
		return http_response_error(kvq, req, param, "参数错误.");;
	}

	//////////////////////////////////////////////////////////////////////////
	modified = db_modify_unsettled_orders(sqlobj_venue_db, where, set);
	_assure_clearbuff(g_membuffer, 64 * 1024);

	if (modified)
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else{
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");
	}

	http_response_ok(kvq, req, param, g_membuffer->data);

	//clear jobs..
	cJSON_Delete(jroot);
	free(reqbody);

	printf("%s", "mod_unsettled_order");
	return eRoute_success;
}

/************************************************************************/
/* ---------setteld querys--------------                                */
/************************************************************************/

static int get_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int num = 0;
	char where[256] = { 0 };
	int  printn = 0;
	settled_order_t * orders = NULL;
	int  count = 0;

	sqlpartstr_fromquery(kvq, g_fields_settled, _arraysize(g_fields_settled), "where", where, sizeof(where));

	int query = db_query_settled_orders(sqlobj_venue_db, where, &orders, &count);

	_assure_clearbuff(g_membuffer, 64 * 1024);

	if (query){
		membuff_add_printf(g_membuffer, "{\"rslt\":\"ok\",\"data\":[");
		for (int i = 0; i < count; i++){
			membuff_add_printf(g_membuffer, "{\"orderdate\":\"%s\",\"orderno\":\"%s\",\"username\":\"%s\",\"age\":\"%d\",\"cardid\":\"%s\",\"telephone\":\"%s\",\"workunit\":\"%s\",\
											  \"email\":\"%s\",\"team\":\"%s\",\"group\":\"%s\",\"teamleader\":\"%s\",\
										      \"operator\":\"%s\",\"opertime\":\"%s\"},",
											  orders[i]->orderdate,
											  orders[i]->orderno,
											  orders[i]->username,
											  orders[i]->age,
											  orders[i]->cardid,
											  orders[i]->telephone,
											  orders[i]->workunit,
											  orders[i]->email,
											  orders[i]->team,
											  orders[i]->group,
											  orders[i]->teamleader,
											  orders[i]->operator,
											  orders[i]->opertime);
		}
		membuff_trim(g_membuffer, ",");
		membuff_addchar(g_membuffer, ']');
		membuff_addchar(g_membuffer, '\0');
	}
	else{
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");
	}

	http_response_ok(kvq, req, param, g_membuffer->data);

	//clear job..
	free(orders);

	printf("%s", "get_settled_order");
	return eRoute_success;
}

static int add_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int    saved = 0;
	cJSON *jroot = NULL;
	int    ordernum = 0;
	int    evbuflen = 0;
	settled_order_t orders = NULL;

	char  *reqbody = http_request_body(req);

	if (reqbody == NULL || strlen(reqbody) == 0 || (jroot = cJSON_Parse((const char*)reqbody)) == NULL){
		if (reqbody) free(reqbody);
		return  http_response_error(kvq, req, param, "参数错误.");
	}

	if ((ordernum = cJSON_GetArraySize(jroot)) == 0)
	{
		free(reqbody);
		return  http_response_error(kvq, req, param, "参数错误.");
	}

	while ((orders = calloc(ordernum, sizeof(settled_order_st))) == NULL) Sleep(1);

	cJSON* obj = NULL;
	for (int i = 0; i < ordernum; i++)
	{
		obj = cJSON_GetArrayItem(jroot, i);

		if (obj == NULL || !_jparse_settledorder(obj, &orders[i]))
		{
			cJSON_Delete(jroot);
			free(orders);
			free(reqbody);
			return  http_response_error(kvq, req, param, "参数错误.");
		}
	}

	saved = db_save_settled_orders(sqlobj_venue_db, orders, ordernum);

	_assure_clearbuff(g_membuffer, 64 * 1024);

	if (saved)
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");

	http_response_ok(kvq, req, param, g_membuffer->data);

	//clear jobs..
	cJSON_Delete(jroot);
	free(orders);
	free(reqbody);

	printf("%s", "add_settled_order");
	return eRoute_success;
}

static int del_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int deleted = 0;
	const char* orderno  = evhttp_find_header(kvq, _field_orderno);
	const char* username = evhttp_find_header(kvq, _field_username);

	if (orderno == NULL) return  http_response_error(kvq, req, param, "参数错误.");;

	//////////////////////////////////////////////////////////////////////////
	deleted = db_delete_settled_orders(sqlobj_venue_db, orderno, username);
	_assure_clearbuff(g_membuffer, 64 * 1024);

	if (deleted)
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else{
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");
	}

	http_response_ok(kvq, req, param, g_membuffer->data);

	printf("%s", "del_settled_order");
	return eRoute_success;
}

static int mod_settled_order(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int    modified = 0;
	cJSON* jroot = NULL;
	char   where[256] = { 0 };
	char   set[256] = { 0 };
	int    formed   = 0;
	char*  reqbody  = http_request_body(req);

	if (reqbody == NULL || strlen(reqbody) == 0 || (jroot = cJSON_Parse((const char*)reqbody)) == NULL) {
		if (reqbody) free(reqbody);
		return  http_response_error(kvq, req, param, "参数错误.");
	}

	cJSON* jwhere = cJSON_GetObjectItem(jroot, "where");
	cJSON* jset   = cJSON_GetObjectItem(jroot, "set");
	if (jset == NULL || jwhere == NULL){
		free(reqbody); cJSON_Delete(jroot);
		return http_response_error(kvq, req, param, "参数错误.");;
	}

	if (!sqlpartstr_fromjson(jwhere, g_fields_settled, _arraysize(g_fields_settled), "where", where, sizeof(where))||
		!sqlpartstr_fromjson(jwhere, g_fields_settled, _arraysize(g_fields_settled), "set", set, sizeof(set)))
	{
		free(reqbody); cJSON_Delete(jroot);
		return http_response_error(kvq, req, param, "参数错误.");;
	}

	//////////////////////////////////////////////////////////////////////////
	modified = db_modify_settled_orders(sqlobj_venue_db, where, set);
	_assure_clearbuff(g_membuffer, 64 * 1024);

	if (modified)
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else{
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");
	}

	http_response_ok(kvq, req, param, g_membuffer->data);

	//clear jobs..
	cJSON_Delete(jroot);
	free(reqbody);

	printf("%s", "mod_settled_order");
	return eRoute_success;
}