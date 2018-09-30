#ifndef __DBACCESS_H__
#define __DBACCESS_H__
#include "mysqldb.h"
#include "scorerslt.h"
#include "statusrslt.h"
#include "scenerslt.h"
#include "struct.h"
#include "visitrslt.h"
//#include "cJSON.h"
#include "json/cJSON.h"
#include "orderquery.h"

int db_load_scores(mysqlquery_t dbinst,int cardid,scores_t scores);

int db_load_scenes(mysqlquery_t dbinst,int sceneid,sceneinfos_t rslt);

int db_load_scenestatus(mysqlquery_t dbinst,int sceneid,int mcuid,char*starttime,char* endtime,scenestatuses_t statuses);

int db_load_visitor_activity(mysqlquery_t dbinst,int cardid,visitrslt_t rslt,int rsltcap);

int db_save_vistor_activity(mysqlquery_t dbinst,int userid,int sceneid,int mcuid,float score);

int db_clear_visitor_activtiy(mysqlquery_t dbinst,int userid);

int db_clear_scores(mysqlquery_t dbinst,int cardid);

int db_load_statistic_scene_scores(mysqlquery_t dbinst,int sceneid,scoredeploy_st*out);

int db_load_statistic_scene_visitors(mysqlquery_t dbinst,int sceneid,agedeploy_st*out);

int db_load_scores_scs(mysqlquery_t dbinst,scores_scs_t scores);


int db_query_remains(mysqlquery_t dbinst, scores_t scores);
int db_query_credits(mysqlquery_t dbinst, int* cnt, int* credits);
int db_query_travelername(mysqlquery_t dbinst, int cardid, char **name);

//////////////////////////////////////////////////////////////////////////
/*
*查询未处理订单
*/
int db_query_unsettled_orders(mysqlquery_t dbinst, char* where,unsettled_order_t ** outorders,int * count);

/*
*增加未处理订单
*/
int db_save_unsettled_orders(mysqlquery_t dbinst,unsettled_order_t orders,int count);

/*
*删除未处理订单
*username == NULL时 则删除所有orderno的订单
*username != NULL时  则删除符合uername条件的订单项目
*/
int db_delete_unsettled_orders(mysqlquery_t dbinst, char* orderno, char* username);

/*
*更改未处理订单
*/
int db_modify_unsettled_orders(mysqlquery_t dbinst, char* where, char* set);

//////////////////////////////////////////////////////////////////////////
/*
*更改已处理订单
*/
int db_query_settled_orders(mysqlquery_t dbinst, char* where, settled_order_t ** outorders, int * count);

/*
*新增已处理订单
*/
int db_save_settled_orders(mysqlquery_t dbinst, settled_order_t orders, int count);

/*
*删除已处理订单
*username == NULL时 则删除所有orderno的订单
*username != NULL时  则删除符合uername条件的订单项目
*/
int db_delete_settled_orders(mysqlquery_t dbinst, char* orderno, char* username);

/*
*更改已处理订单
*username == NULL时 则删除所有orderno的订单
*username != NULL时  则删除符合uername条件的订单项目
*/
int db_modify_settled_orders(mysqlquery_t dbinst, char* where, char* set);

#endif