#include "cardquery.h"
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
#include "visitrslt.h"
#include "scorerslt.h"
#include "statusrslt.h"
#include "scenerslt.h"
#include "appconfig.h"
#include "struct.h"
#include "mysqldb.h"
#include "dbaccess.h"
#include "httpquery_helper.h"

extern config_st    g_conf;
extern mysqlquery_t sqlobj_venue_db;
static membuff_t    g_membuffer = NULL;

#define NULL 0

static char*  g_helpstr = NULL;
extern log_t  g_log;

static int card_add(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int card_del(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int card_mod(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int card_get(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int card_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);

static route_entry_st route_map[] = {
	{ "/card/add", card_add,"" },
	{ "/card/del", card_del,"" },
	{ "/card/mod", card_mod,"" },
	{ "/card/get", card_get, "" },
	{ "/card/help", card_help,"" },
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

	_assure_clearbuff(g_membuffer,1024);

	membuff_add_printf(g_membuffer, "{\"type\":\"help\",\"code\":\"-1\",\"rslt\":");
	membuff_add_printf(g_membuffer, "\"%s\"", g_helpstr);
	membuff_addchar(g_membuffer, '}');
	membuff_addchar(g_membuffer, '\0');

	http_response_ok(kvq, req,param, g_membuffer->data);

	return eRoute_success;
}

cardquery_t cardquery_new()
{
	cardquery_t rt = calloc(1, sizeof(cardquery_st));
	rt->_base.name = "card";
	rt->_base.entries = route_map;
	rt->_base._free = cardquery_free;
	rt->_base._help = card_help;
	rt->_base.entrycount = sizeof(route_map) / sizeof(route_map[0]);

	return rt;
}

void   cardquery_free(cardquery_t query)
{
	if (query) free(query);
}

//////////////////////////////////////////////////////////////////////////
//get method.
int card_add(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	const char* cardid = evhttp_find_header(kvq, "cardid");
	const char* cardsn = evhttp_find_header(kvq, "cardsn");

	if (cardid == NULL || req == NULL || strlen(cardid) == 0){ return eRoute_failed;}

	_assure_clearbuff(g_membuffer, 4096);

	if (db_save_card(sqlobj_venue_db, cardid, cardsn))
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");

	http_response_ok(kvq, req,param, g_membuffer->data);

	return eRoute_success;
}

//get method.
//删除卡片.
int card_del(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	const char* cardid = evhttp_find_header(kvq, "cardid");
	const char* cardsn = evhttp_find_header(kvq, "cardsn");

	if (req == NULL) return eRoute_failed; 

	_assure_clearbuff(g_membuffer, 4096);

	if (db_del_card(sqlobj_venue_db, cardid, cardsn))
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");

	http_response_ok(kvq, req, param, g_membuffer->data);

	return eRoute_success;
}

//get method.
//更改卡片.
int card_mod(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	const char* cardid = evhttp_find_header(kvq, "cardid");
	const char* cardsn = evhttp_find_header(kvq, "cardsn");
	const char* direction = evhttp_find_header(kvq, "direction");
	int  mod = 0;

	if ((cardid == NULL && cardsn == NULL) || req == NULL || direction == NULL){
		return http_response_error(kvq, req,param,"参数错误.");
	}

	_assure_clearbuff(g_membuffer, 4096);

	if (strcmp("id2sn", direction) == 0)
		mod = db_mod_card_byid(sqlobj_venue_db, cardid, cardsn);
	else if (strcmp("sn2id", direction) == 0)
		mod = db_mod_card_bysn(sqlobj_venue_db, cardid, cardsn);
	else{
		return http_response_error(kvq, req, param, "参数错误.");
	}

	if (mod)
		membuff_add_printf(g_membuffer, "{\"rslt\":\"success\"}");
	else
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");

	http_response_ok(kvq, req, param, g_membuffer->data);

	return eRoute_success;
}

//get method.
//查询卡片.
int card_get(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	const char* cardid = evhttp_find_header(kvq, "cardid");
	const char* cardsn = evhttp_find_header(kvq, "cardsn");
	int  getted = 0;
	int  count  = 0;
	card_t cards = NULL;

	if (req == NULL) return http_response_error(kvq, req, param, "参数错误.");

	_assure_clearbuff(g_membuffer, 4096);

	getted = db_query_card(sqlobj_venue_db, cardid, cardsn, &cards, &count);

	if (getted){
		membuff_add_printf(g_membuffer, "{\"rslt\":\"ok\",\"data\":[");
		for (int i = 0; i < count; i++)
			membuff_add_printf(g_membuffer, "{\"cardid\":\"%s\",\"cardsn\":\"%s\",\"uptime\":\"%s\",\"ctime\":\"%s\"},",\
				               cards[i].cardid, 
							   cards->cardsn, 
							   cards->utime, 
							   cards->ctime);	//should be "".									  		
		
		membuff_trim(g_membuffer, ",");
		membuff_addchar(g_membuffer,'\0');
	}
	else
		membuff_add_printf(g_membuffer, "{\"rslt\":\"failed\",\"reason\":\"数据库操作失败！\"}");

	http_response_ok(kvq, req, param, g_membuffer->data);

	return eRoute_success;
}

int card_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
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

	if (req == NULL) { return http_response_error(kvq, req, param, "查询错误."); }

	_assure_clearbuff(g_membuffer,4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"help\",\"code\":\"-1\",\"rslt\":");
	membuff_add_printf(g_membuffer, "\"%s\"", g_helpstr);
	membuff_addstr(g_membuffer, "}",sizeof("}"));

	http_response_ok(kvq, req, param, g_membuffer->data);

	return eRoute_success;
}
