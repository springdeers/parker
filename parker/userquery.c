#include "userquery.h"
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

#include "struct.h"
#include "mysqldb.h"
#include "dbaccess.h"

extern mysqlquery_t sqlobj_venue_db;
extern mysqlquery_t sqlobj_userinfo_db;
static membuff_t g_membuffer = NULL;

#define NULL 0

static char*  g_helpstr = NULL;
extern log_t  g_log;
DWORD         score_scs_update_time = 0;
scores_scs_st scores_scs;

static int user_test(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int user_score(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int user_backcard(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int user_about(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int user_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int user_scenestatus(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int user_scenes(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int user_useract(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);
static int user_analyscene(struct evkeyvalq*kvq, struct evhttp_request* req, void* param);

static route_entry_st route_map[] = {
	{ "/user/score", user_score, "" },
	{ "/user/backcard", user_backcard, "" },
	{ "/user/scenestatus", user_scenestatus, "" },
	{ "/user/scenes", user_scenes, "" },
	{ "/user/useract", user_useract, "" },
	{ "/user/analyscene", user_analyscene, "" },
	{ "/user/about", user_about, "" },
	{ "/user/help", user_help, "" },
	{ NULL, NULL },
};

int user_help(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
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

	if (req == NULL) {evbuffer_free(buf); return eRoute_failed;}

	_assure_clearbuff(g_membuffer, 4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"help\",\"code\":\"-1\",\"rslt\":");
	membuff_add_printf(g_membuffer, "\"%s\"", g_helpstr);
	membuff_addstr(g_membuffer, "}", sizeof("}"));

	evbuffer_add_printf(buf, g_membuffer->data);
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);

	return eRoute_success;
}

userquery_t userquery_new()
{
	userquery_t rt = calloc(1, sizeof(userquery_st));
	rt->_base.name = "user";
	rt->_base.entries = route_map;
	rt->_base._free = userquery_free;
	rt->_base._help = user_help;
	rt->_base.entrycount = sizeof(route_map )/ sizeof(route_map[0]);

	return rt;
}

void   userquery_free(userquery_t query)
{
	if (query) free(query);
}
//////////////////////////////////////////////////////////////////////////

int user_analyscene(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int i = 0;
	scoredeploy_st scoredeploy;
	agedeploy_st   agedeploy;

	struct evbuffer* buf = NULL;
	const char* sceneid = evhttp_find_header(kvq, "sceneid");

	while ((buf = evbuffer_new()) == NULL) Sleep(1);

	if (sceneid == NULL || req == NULL || strlen(sceneid) == 0){ evbuffer_free(buf); return eRoute_failed; }

	agedeploy.sceneid = 0;
	agedeploy.buffer[0] = '\0';

	scoredeploy.sceneid = 0;
	scoredeploy.buffer[0] = '\0';

	db_load_statistic_scene_scores(sqlobj_venue_db, atoi(sceneid), &scoredeploy);
	db_load_statistic_scene_visitors(sqlobj_venue_db, atoi(sceneid), &agedeploy);

	_assure_clearbuff(g_membuffer, 4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"analyscene\",\"code\":\"0\",\"sceneid\":\"%s\",\"rslt\":", sceneid);
	membuff_add_printf(g_membuffer, "%c", '[');

	membuff_add_printf(g_membuffer, "{\"score\":%s},{\"age\":%s}", scoredeploy.buffer, agedeploy.buffer);

	membuff_addstr(g_membuffer, "]}", sizeof("]}"));

	evbuffer_add_printf(buf, membuff_data(g_membuffer));

	//回复给客户端
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);

	return eRoute_success;
}

int user_score(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int i = 0, score_scs_freeflag = 0;
	scores_st scores;
	struct evbuffer* buf = NULL;
	const char* cardid = evhttp_find_header(kvq, "cardid");
	const char* comid = evhttp_find_header(kvq, "comid");		// 此处无用，只是为了向printsvr转发时方便
	//const char* queryonly = evhttp_find_header(kvq, "queryonly");

	while ((buf = evbuffer_new()) == NULL)Sleep(1);

	if (cardid == NULL || req == NULL || strlen(cardid) == 0) { evbuffer_free(buf); return eRoute_failed; }

	db_load_scores(sqlobj_venue_db, atoi(cardid), &scores);		// 将cardid对应的所有成绩记录全部取出来，放到scores结构体中
	scores_shrink(&scores);			// 对scores结构体进行精简，同一个场景只出唯一的成绩（取最大值）

	// 获取所有场景个数与学分总和
	if (-1 == db_query_credits(sqlobj_venue_db, &scores.totalscenes, &scores.totalcredit))
	{
		evbuffer_free(buf);
		printf("db_query_credits failed.\n");
		return -1;
	}

	if (scores.totalcredit == 0)
	{
		evbuffer_free(buf);
		printf("error: scores.totalcredit == 0.\n");
		return -1;
	}

	// 获取未体验场景，构造出json串
	// json串格式： {"remains":[{"name":"烟雾逃生"},{"name":"汽车落水"},{"name":"地震小屋"}]}
	if (-1 == db_query_remains(sqlobj_venue_db, &scores))
	{
		evbuffer_free(buf);
		printf("db_query_credits failed.\n");
		return -1;
	}

	// 获取游客姓名
	char* name = NULL;
	if (-1 == db_query_travelrname(sqlobj_userinfo_db, atoi(cardid), &name))
	{
		evbuffer_free(buf);
		printf("db_query_travelrname failed.\n");
		return -1;
	}
		
	// 最重要的改动，最新成绩算法
	finalscore_st finalscore = {0};
	memcpy(finalscore.name, name, strlen(name) + 1);
	free(name);
	get_finalscore(&scores, &finalscore);

	_assure_clearbuff(g_membuffer,4096);

	membuff_add_printf(g_membuffer, "{\"name\":\"%s\",\"type\":\"score\",\"code\":\"0\",\"cardid\":\"%s\",\"comid\":\"%s\",\"finalscore\":\"%.1f\",\"credit_point\":\"%.1f\",\"grade_point\":\"%.1f\",\"class\":\"%s\",\"suggestion\":\"%s\",\"remainsobj\":%s}",
		finalscore.name,
		cardid,
		comid,
		finalscore.finalscore,
		finalscore.credit_point,
		finalscore.grade_point,
		finalscore.scoreclass,
		finalscore.suggestion,
		finalscore.rmnscenes
		);					

	membuff_addchar(g_membuffer, '\0');

	evbuffer_add_printf(buf, membuff_data(g_membuffer));
	printf("reply to client:\n %s\n", g_membuffer->data);
	//回复给客户端
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gbk");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	

	evbuffer_free(buf);

	scores_freeitems(&scores);

	/*if (queryonly && strcmp(queryonly, "true") == 0)
		return eRoute_success;

	db_clear_scores(sqlobj_venue_db, atoi(cardid));
*/
	return eRoute_success;
}


int user_backcard(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int i = 0, score_scs_freeflag = 0;
	scores_st scores;
	struct evbuffer* buf = NULL;
	const char* cardid = evhttp_find_header(kvq, "cardid");
	const char* comid = evhttp_find_header(kvq, "comid");		// 此处无用，只是为了向printsvr转发时方便
	//const char* queryonly = evhttp_find_header(kvq, "queryonly");

	while ((buf = evbuffer_new()) == NULL)Sleep(1);

	if (cardid == NULL || req == NULL || strlen(cardid) == 0) { evbuffer_free(buf); return eRoute_failed; }

	db_load_scores(sqlobj_venue_db, atoi(cardid), &scores);		// 将cardid对应的所有成绩记录全部取出来，放到scores结构体中
	scores_shrink(&scores);			// 对scores结构体进行精简，同一个场景只出唯一的成绩（取最大值）

	// 获取所有场景个数与学分总和
	if (-1 == db_query_credits(sqlobj_venue_db, &scores.totalscenes, &scores.totalcredit))
	{
		evbuffer_free(buf);
		printf("db_query_credits failed.\n");
		return -1;
	}

	if (scores.totalcredit == 0)
	{
		evbuffer_free(buf);
		printf("error: scores.totalcredit == 0.\n");
		return -1;
	}

	// 获取未体验场景，构造出json串
	// json串格式： {"remains":[{"name":"烟雾逃生"},{"name":"汽车落水"},{"name":"地震小屋"}]}
	if (-1 == db_query_remains(sqlobj_venue_db, &scores))
	{
		evbuffer_free(buf);
		printf("db_query_credits failed.\n");
		return -1;
	}

	char* name = NULL;
	if (-1 == db_query_travelrname(sqlobj_userinfo_db, atoi(cardid), &name))
	{
		evbuffer_free(buf);
		printf("db_query_travelrname failed.\n");
		return -1;
	}

	// 最重要的改动，最新成绩算法
	finalscore_st finalscore = { 0 };
	memcpy(finalscore.name, name, strlen(name) + 1);
	free(name);
	get_finalscore(&scores, &finalscore);

	_assure_clearbuff(g_membuffer, 4096);
	
	/*membuff_add_printf(g_membuffer, "{\"name\":\"%s\",\"type\":\"score\",\"code\":\"0\",\"cardid\":\"%s\",\"comid\":\"%s\",\"finalscore\":\"%.1f\",\"credit_point\":\"%.1f\",\"grade_point\":\"%.1f\",\"class\":\"%s\",\"suggestion\":\"%s\",\"remainsobj\":%s}",
		scores.name,
		cardid,
		comid,
		finalscore.finalscore,
		finalscore.credit_point,
		finalscore.grade_point,
		finalscore.scoreclass,
		finalscore.suggestion,
		finalscore.rmnscenes
		);*/
	membuff_add_printf(g_membuffer, "{\"name\":\"%s\",\"type\":\"backcard\",\"rslt\":\"ok\",\"cardid\":\"%s\",\"comid\":\"%s\"}",
		finalscore.name,
		cardid,
		comid
		);

	membuff_addchar(g_membuffer, '\0');

	evbuffer_add_printf(buf, membuff_data(g_membuffer));
	printf("reply to client:\n %s\n", g_membuffer->data);
	//回复给客户端
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gbk");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);

	scores_freeitems(&scores);

	/*if (queryonly && strcmp(queryonly, "true") == 0)
	return eRoute_success;

	db_clear_scores(sqlobj_venue_db, atoi(cardid));
	*/
	return eRoute_success;
}


int user_about(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	struct evbuffer* buf = NULL;
	while ((buf = evbuffer_new()) == NULL) Sleep(1);

	if (req == NULL || kvq == NULL) { evbuffer_free(buf); return eRoute_failed; }

	_assure_clearbuff(g_membuffer, 4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"help\",\"code\":\"-1\",\"rslt\":");
	membuff_add_printf(g_membuffer, "\"%s\"", "score query api.");
	membuff_addstr(g_membuffer, "}", sizeof("}"));

	evbuffer_add_printf(buf, membuff_data(g_membuffer));
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);

	return eRoute_success;
}


int user_scenestatus(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int i = 0;
	scenestatuses_t statuses = scenestatuses_new();
	struct evbuffer* buf = NULL;
	const char* sceneid = evhttp_find_header(kvq, "sceneid");
	const char* mcuid = evhttp_find_header(kvq, "mcuid");
	const char* starttime = evhttp_find_header(kvq, "starttime");
	const char* endtime = evhttp_find_header(kvq, "endtime");

	while ((buf = evbuffer_new())==NULL) Sleep(1);

	if (sceneid == NULL || req == NULL || strlen(sceneid) == 0){ evbuffer_free(buf); return eRoute_failed; }

	db_load_scenestatus(sqlobj_venue_db, atoi(sceneid), mcuid == NULL ? 0 : atoi(mcuid), starttime, endtime, statuses);
	
	_assure_clearbuff(g_membuffer,4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"scenestatus\",\"code\":\"0\",\"rslt\":");
	membuff_add_printf(g_membuffer, "%c", '[');

	for (; i < statuses->statusnum; i++){
		membuff_add_printf(g_membuffer, "%C", '{');
		membuff_add_printf(g_membuffer, "\"sceneid\":\"%d\",\"mcuid\":\"%d\",\
									    \"type\":\"%d\",\"battery\":\"%d\",\"mcustate\":\"%d\",\
										\"sensorstate\":\"%d\",\"sensortype\":\"%d\",\"time\":\"%s\"",
										statuses->status[i].sceneid, statuses->status[i].mcuid, statuses->status[i].type, statuses->status[i].battery, \
										statuses->status[i].mcustate, statuses->status[i].sensorstate, statuses->status[i].sensortype, statuses->status[i].time);

		membuff_addstr(g_membuffer, "},", strlen("},"));
	}

	if (g_membuffer->data[g_membuffer->len - 1] == ',') g_membuffer->len--;

	membuff_addstr(g_membuffer, "]}", sizeof("]}"));

	evbuffer_add_printf(buf, g_membuffer->data);

	//回复给客户端
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);

	scenestatuses_free(statuses);

	return eRoute_success;
}

static int user_useract(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int i = 0;
	visitrslt_st rslts[128];

	int num = 0;
	struct evbuffer* buf  = NULL;
	const char* cardid    = evhttp_find_header(kvq, "cardid");
	const char* queryonly = evhttp_find_header(kvq, "queryonly");

	while ((buf = evbuffer_new()) == NULL) Sleep(1);

	if (cardid == NULL || req == NULL || strlen(cardid) == 0){ evbuffer_free(buf); return eRoute_failed; }

	num = db_load_visitor_activity(sqlobj_venue_db, atoi(cardid), rslts, sizeof(rslts) / sizeof(rslts[0]));

	if (g_membuffer == NULL) g_membuffer = membuff_new(512);
	membuff_clear(g_membuffer);

	membuff_add_printf(g_membuffer, "{\"type\":\"useract\",\"code\":\"0\",\"rslt\":");
	membuff_add_printf(g_membuffer, "%c", '[');

	for (; i < num; i++)
		membuff_add_printf(g_membuffer, "{\"sceneid\":\"%d\",\"scenename\":\"%s\",\"traveltime\":\"%s\"},", rslts[i].id, rslts[i].name, rslts[i].time);

	if (g_membuffer->data[g_membuffer->len - 1] == ',') g_membuffer->len--;
	membuff_addstr(g_membuffer, "]}", sizeof("]}"));

	evbuffer_add_printf(buf, g_membuffer->data);

	//回复给客户端
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);

	evbuffer_free(buf);

	if (queryonly && strcmp(queryonly, "true") == 0)
		return eRoute_success;

	db_clear_visitor_activtiy(sqlobj_venue_db, atoi(cardid));

	return eRoute_success;
}

int user_scenes(struct evkeyvalq*kvq, struct evhttp_request* req, void* param)
{
	int i = 0;
	sceneinfos_t   rslts = sceneinfos_new();
	struct evbuffer* buf = NULL;
	const char* sceneid = evhttp_find_header(kvq, "sceneid");

	while ((buf = evbuffer_new()) == NULL) Sleep(1);

	if (sceneid == NULL || req == NULL || strlen(sceneid) == 0) { evbuffer_free(buf); return eRoute_failed; }

	db_load_scenes(sqlobj_venue_db, atoi(sceneid), rslts);

	_assure_clearbuff(g_membuffer,4096);

	membuff_add_printf(g_membuffer, "{\"type\":\"sceneinfo\",\"code\":\"0\",\"rslt\":");
	membuff_add_printf(g_membuffer, "%c", '[');

	for (; i < rslts->num; i++){
		membuff_add_printf(g_membuffer, "%C", '{');
		membuff_add_printf(g_membuffer, "\"sceneid\":\"%d\",\"name\":\"%s\",\"template\":%s",
			rslts->infos[i]->sceneid, rslts->infos[i]->scenename, rslts->infos[i]->templ);

		membuff_add_printf(g_membuffer, "%C,", '}');
	}

	if (g_membuffer->data[g_membuffer->len - 1] == ',') g_membuffer->len--;
	membuff_addstr(g_membuffer, "]}", sizeof("]}"));

	evbuffer_add_printf(buf, g_membuffer->data);

	//回复给客户端
	evhttp_add_header(req->output_headers, "Content-Type", "text/json;charset=gb2312");
	evhttp_send_reply(req, HTTP_OK, "OK", buf);
	evbuffer_free(buf);

	sceneinfos_free(rslts);

	return eRoute_success;
}
