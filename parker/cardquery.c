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

extern config_st    g_conf;
extern mysqlquery_t sqlobj_venue_db;
static membuff_t    g_membuffer = NULL;

#define NULL 0

static char*  g_helpstr = NULL;
extern log_t  g_log;
extern DWORD         score_scs_update_time;
extern scores_scs_st scores_scs;

static int card_insert(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int card_about(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int card_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);


static route_entry_st route_map[] = {
	{ "/card/insert", card_insert, "" },
	{ "/card/about", card_about, "" },
	{ "/card/help", card_help, "" },
	{ NULL, NULL },
};

int route_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	struct evbuffer* buf = NULL;

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

	while ((buf =evbuffer_new()) == NULL) Sleep(1);

	if (req == NULL) { evbuffer_free(buf); return eRoute_failed; }

	_assure_clearbuff(g_membuffer,1024);

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

int card_insert(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int i = 0, score_scs_freeflag = 0;
	scores_st scores;
	char * scsstr;
	struct evbuffer* buf = NULL;
	const char* cardid = evhttp_find_header(kvq, "cardid");
	const char* cardsn = evhttp_find_header(kvq, "cardsn");

	while ((buf = evbuffer_new()) == NULL) Sleep(1);
	if (cardid == NULL || req == NULL || strlen(cardid) == 0){evbuffer_free(buf); return eRoute_failed;}

	//db_load_scores(sqlobj_venue_db, atoi(cardid), &scores);
	//db_card_insert(sqlobj_venue_db, atoi(cardid), cardsn);
	//scores_shrink(&scores);

	if (GetTickCount() - score_scs_update_time > g_conf.score_scs_refresh){
		scores_scs_free(&scores_scs);
		db_load_scores_scs(sqlobj_venue_db, &scores_scs);
		score_scs_update_time = GetTickCount();
		printf("refresh db_load_scores_scs..(%d)\n", score_scs_update_time);
	}


	_assure_clearbuff(g_membuffer, 4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"score\",\"code\":\"0\",\"iccard\":\"%s\",\"rslt\":", cardid);
	membuff_add_printf(g_membuffer, "%c", '[');

	for (; i < scores.scorenum; i++){//?
		scsstr = replace_score_suggest(&scores.score[i], &scores_scs);
		membuff_add_printf(g_membuffer, "%s,", scsstr);
		if (scsstr)  free(scsstr);
		//membuff_add_printf(g_membuffer,"%s,",scores.score[i].json);
	}

	if (g_membuffer->data[g_membuffer->len - 1] == ',') g_membuffer->len--;
	membuff_addstr(g_membuffer, "]}", sizeof("]}"));

	evbuffer_add_printf(buf, membuff_data(g_membuffer));

	//回复给客户端
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);

	scores_freeitems(&scores);

	/*if (queryonly && strcmp(queryonly, "true") == 0)
		return eRoute_success;*/

	db_clear_scores(sqlobj_venue_db, atoi(cardid));

	return eRoute_success;
}

int card_about(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	struct evbuffer* buf = evbuffer_new();
	if (buf == NULL) return eRoute_failed;

	if (req == NULL || kvq == NULL) {evbuffer_free(buf); return eRoute_failed;}

	_assure_clearbuff(g_membuffer, 4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"help\",\"code\":\"-1\",\"rslt\":");
	membuff_add_printf(g_membuffer, "\"%s\"", "score query api.");
	membuff_addstr(g_membuffer, "}",sizeof("}"));

	evbuffer_add_printf(buf, membuff_data(g_membuffer));
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);

	return eRoute_success;
}


int card_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	struct evbuffer* buf = NULL;
	while ((buf = evbuffer_new()) == NULL) Sleep(1);

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

	if (req == NULL) { evbuffer_free(buf); return eRoute_failed; }

	_assure_clearbuff(g_membuffer,4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"help\",\"code\":\"-1\",\"rslt\":");
	membuff_add_printf(g_membuffer, "\"%s\"", g_helpstr);
	membuff_addstr(g_membuffer, "}",sizeof("}"));

	evbuffer_add_printf(buf, g_membuffer->data);
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);

	return eRoute_success;
}
