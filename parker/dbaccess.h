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
#include "cardquery.h"

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
*��ѯδ������
*/
int db_query_unsettled_orders(mysqlquery_t dbinst, char* where,unsettled_order_t ** outorders,int * count);

/*
*����δ������
*/
int db_save_unsettled_orders(mysqlquery_t dbinst,unsettled_order_t orders,int count);

/*
*ɾ��δ������
*username == NULLʱ ��ɾ������orderno�Ķ���
*username != NULLʱ  ��ɾ������uername�����Ķ�����Ŀ
*/
int db_delete_unsettled_orders(mysqlquery_t dbinst, char* orderno, char* username);

/*
*����δ������
*/
int db_modify_unsettled_orders(mysqlquery_t dbinst, char* where, char* set);

//////////////////////////////////////////////////////////////////////////
/*
*�����Ѵ�����
*/
int db_query_settled_orders(mysqlquery_t dbinst, char* where, settled_order_t ** outorders, int * count);

/*
*�����Ѵ�����
*/
int db_save_settled_orders(mysqlquery_t dbinst, settled_order_t orders, int count);

/*
*ɾ���Ѵ�����
*username == NULLʱ ��ɾ������orderno�Ķ���
*username != NULLʱ  ��ɾ������uername�����Ķ�����Ŀ
*/
int db_delete_settled_orders(mysqlquery_t dbinst, char* orderno, char* username);

/*
*�����Ѵ�����
*username == NULLʱ ��ɾ������orderno�Ķ���
*username != NULLʱ  ��ɾ������uername�����Ķ�����Ŀ
*/
int db_modify_settled_orders(mysqlquery_t dbinst, char* where, char* set);

//////////////////////////////////////////////////////////////////////////
/*
*������Ƭ��Ϣ��
*����1���ɹ���0��ʧ�ܡ�
*/
int db_save_card(mysqlquery_t dbinst, char * cardid, char* cardsn);

/*
*ɾ����Ƭ��Ϣ��
*cardid��ΪNULL,cardsn��ΪNULL;��cardid=NULl����cardsn=NULLʱ��������м�¼.
*����1���ɹ���0��ʧ�ܡ�
*/
int db_del_card(mysqlquery_t dbinst, char * cardid, char* cardsn);

/*
*����cardid���Ŀ�Ƭ��Ϣ��
*cardid��ΪNULL,cardsn��ΪNULL;
*����1���ɹ���0��ʧ�ܡ�
*/
int db_mod_card_byid(mysqlquery_t dbinst, char * cardid, char* cardsn);

/*
*����cardsn���Ŀ�Ƭ��Ϣ��
*cardid��ΪNULL,cardsn��ΪNULL;
*����1���ɹ���0��ʧ�ܡ�
*/
int db_mod_card_bysn(mysqlquery_t dbinst, char * cardid, char* cardsn);

/*
*��ѯ��Ƭ��Ϣ��
*cardid��ΪNULL,cardsn��ΪNULL;���߾�ΪNULLʱ���������м�¼.
*����1���ɹ���0��ʧ�ܡ�
*/
int db_query_card(mysqlquery_t dbinst, char*cardid,char* cardsn,card_t* cards, int *count);

#endif