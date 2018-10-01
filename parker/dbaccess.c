#include "dbaccess.h"
#include <stdio.h>
#include "tool.h"
#include "membuff.h"

extern mysqlquery_t sqlobj_venue_db;


int db_query_credits(mysqlquery_t dbinst, int* cnt, int *credits)
{
	char sql[512] = { 0 };
	int  rt = -1;

	if (dbinst == NULL)
		return rt;

	sprintf_s(sql, sizeof(sql) - 1, "SELECT COUNT(*),SUM(credit) FROM tbScene");
	rt = mysql_real_query(dbinst->m_con, sql, strlen(sql));

	if (rt == 0)
	{
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while (dbinst->m_row = mysql_fetch_row(dbinst->m_res))
		{
			if (dbinst->m_row[0])
				*cnt = atoi(dbinst->m_row[0]);

			if (dbinst->m_row[1])
				*credits = atoi(dbinst->m_row[1]);
		}
		mysql_free_result(dbinst->m_res);
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_query_scene_cnt . query error :%s", mysql_error(dbinst->m_con));
	}

	return rt;
}


// 从user_info_db中获取游客姓名即nickname
int db_query_travelername(mysqlquery_t dbinst, int cardid, char **name)
{
	char sql[512] = { 0 };
	int  rt = -1;

	if (dbinst == NULL)
		return rt;

	sprintf_s(sql, sizeof(sql) - 1, "SELECT nickame FROM tbtraveler WHERE userid=%d", cardid);
	int query = mysql_real_query(dbinst->m_con, sql, strlen(sql));

	if (query == 0)
	{
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while (dbinst->m_row = mysql_fetch_row(dbinst->m_res))
		{
			if (dbinst->m_row[0])
			{
				*name = _strdup(dbinst->m_row[0]);
				rt = 0;
			}	
		}
		mysql_free_result(dbinst->m_res);
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_query_travelrname . query error :%s", mysql_error(dbinst->m_con));
	}

	return rt;
}


int db_query_remains(mysqlquery_t dbinst, scores_t scores)
{
	char sql[512] = { 0 };
	char expression[512] = { 0 };
	char remains[512] = { 0 };
	int  rt = -1;
	char *scenename = NULL;

	if (dbinst == NULL)
		return rt;

	int i = 0;
	int printnum = 0;
	for (i = 0; i < scores->scorenum-1; i++)
	{
		printnum += sprintf_s(expression + printnum, sizeof(expression) - 1 - printnum , "_id != %d && ", scores->score[i].sceneid);
	}
	printnum += sprintf_s(expression+printnum, sizeof(expression) - 1 - printnum , "_id != %d ", scores->score[i].sceneid);

	sprintf(sql, "SELECT _name FROM tbScene WHERE %s ",expression);
	int query = mysql_real_query(dbinst->m_con, sql, strlen(sql));

	if (query == 0)
	{
		dbinst->m_res = mysql_store_result(dbinst->m_con);

		cJSON * root = cJSON_CreateObject();
		cJSON * remain_array = cJSON_CreateArray();
		

		while (dbinst->m_row = mysql_fetch_row(dbinst->m_res))
		{
			if (dbinst->m_row[0])
				scenename = _strdup(dbinst->m_row[0]);

			//char arr[30];
			//sprintf(arr, "{\"name\":\"%s\"} ", scenename);

			cJSON * arr_item = cJSON_CreateObject();
			cJSON_AddItemToObject(arr_item, "name", cJSON_CreateString(scenename));
			cJSON_AddItemToArray(remain_array, arr_item);

			rt = 0;
		}

		if (rt != 0)
		{
			cJSON_Delete(root);
			cJSON_Delete(remain_array);
		}
		else
		{
			// 形成最终的json串，并赋值给scores->remains
			cJSON_AddItemToObject(root, "remains", remain_array);

			char * rslt = cJSON_PrintUnformatted(root);
			strcpy_s(scores->remains, strlen(rslt) + 1, rslt);
			free(rslt);

			cJSON_Delete(root);
		}
		
		mysql_free_result(dbinst->m_res);
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_query_remains . query error :%s", mysql_error(dbinst->m_con));
	}

	return rt;
}

int db_load_scores(mysqlquery_t dbinst,int cardid,scores_t scores)
{
	char sql[512] = {0};
	int  rt = 1;
	
	if(dbinst == NULL || cardid <0 )
		return rt;
	
	scores_clear(scores);

	sprintf(sql,"SELECT _sceneid,_name,_cardid,_score,_state,_secondcost,_json,credit FROM tbVisitorScore,tbScene \
				WHERE _cardid=%d AND tbscene._id=tbVisitorScore._sceneid",cardid);
	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt == 0)
	{
		score_st score;
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while(dbinst->m_row = mysql_fetch_row(dbinst->m_res))
		{
			if(dbinst->m_row[0])
				score.sceneid = atoi(dbinst->m_row[0]);	

			if(dbinst->m_row[1])
				score.scenename = _strdup(dbinst->m_row[1]);

			if(dbinst->m_row[2])
				score.cardid= atoi(dbinst->m_row[2]);							

			if(dbinst->m_row[3])
				score.fscore = atof(dbinst->m_row[3]);							

			if(dbinst->m_row[4])
				score.state = atoi(dbinst->m_row[4]);		

			if(dbinst->m_row[5])
				score.secondcost = atoi(dbinst->m_row[5]);		

			if(dbinst->m_row[6])
				score.json = _strdup(dbinst->m_row[6]);		

			if (dbinst->m_row[7])
				score.credit = atoi(dbinst->m_row[7]);

			scores_add(scores,&score);
		}
		mysql_free_result(dbinst->m_res);	

		//删除记录
		//sprintf(sql,"DELETE FROM tbVisitorScore WHERE _cardid=%d",cardid);
		//rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));
	}else{
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scores . query error :%s",mysql_error(dbinst->m_con));		
	}

	return rt;
}


int db_load_scores_scs(mysqlquery_t dbinst,scores_scs_t scores){
	
	char sql[512] = {0};
	int  rt = 1;
	
	int scenei,strlength;
	char * json;
	cJSON *cj_root,*cj_scores,*cj_score,*cj_s1,*cj_s2,*cj_content,*cj_class;
	int cj_scoresize;
	int i;

	memset(scores,0,sizeof(scores_scs_st));

	//printf("SELECT _id,scorerange FROM tbScene");
	sprintf(sql,"SELECT _id,scorerange FROM tbScene");
	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt == 0)
	{
		//score_ran_st score;
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while(dbinst->m_row = mysql_fetch_row(dbinst->m_res))
		{
			if(dbinst->m_row[0])
				scenei = atoi(dbinst->m_row[0]);	
			if(dbinst->m_row[1])
				json = _strdup(dbinst->m_row[1]);
			cj_root = cJSON_Parse(json);
			if(cj_root){
				cj_scores = cJSON_GetObjectItem(cj_root,"scorerange");
				if(cj_scores){
					cj_scoresize = cJSON_GetArraySize(cj_scores);
					for(i=0;i<cj_scoresize;i++){
						score_ran_st score;
						score.sceneid = scenei;
						cj_score = cJSON_GetArrayItem(cj_scores,i);
						if(cj_score){
							cj_s1 = cJSON_GetObjectItem(cj_score,"s1");
							cj_s2 = cJSON_GetObjectItem(cj_score,"s2");
							cj_content = cJSON_GetObjectItem(cj_score,"content");
							cj_class = cJSON_GetObjectItem(cj_score,"classg");
							if(cj_s1){
								score.s1 = atof(cj_s1->valuestring);;
							}
							if(cj_s2){
								score.s2 = atof(cj_s2->valuestring);
							}
							if(cj_content){
								strlength = strlen(cj_content->valuestring)+1;
								score.c_sug = malloc(strlength);
								memcpy(score.c_sug,cj_content->valuestring,strlength);
							}
							if(cj_class){
								strlength = strlen(cj_class->valuestring)+1;
								score.c_class = malloc(strlength);
								memcpy(score.c_class,cj_class->valuestring,strlength);
							}
							if(scores->scorenum<_maxscore_scs_num){
								scores->score[scores->scorenum++] = score;
							}
						}
					}
				}
				cJSON_Delete(cj_root);
			}
		}
		mysql_free_result(dbinst->m_res);	

		//删除记录
		//sprintf(sql,"DELETE FROM tbVisitorScore WHERE _cardid=%d",cardid);
		//rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));
	}else{
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scores_scs . query error :%s",mysql_error(dbinst->m_con));		
	}

	return rt;

}


int db_clear_scores(mysqlquery_t dbinst,int cardid)
{
	char sql[512] = {0};
	int  rt = 1;

	if(dbinst == NULL || cardid <0 )
		return rt;

	sprintf(sql,"DELETE FROM tbVisitorScore \
				 WHERE _cardid=%d",cardid);

	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt != 0){
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scores . query error :%s",mysql_error(dbinst->m_con));		
	}

	return rt;
}

int db_load_scenestatus(mysqlquery_t dbinst,int sceneid,int mcuid,char*starttime,char* endtime,scenestatuses_t statuses)
{
	char sql[512] = {0};
	int  rt = 1;
	int printn = 0;

	if(dbinst == NULL || sceneid <0 || statuses == NULL)
		return rt;

	scenestatuses_clear(statuses);

	printn += sprintf(sql+printn,"SELECT _sceneid,_mcuid,_type,_battery,_state,_sensorstate,_sensortype,_time FROM tbSceneStatus WHERE _sceneid=%d ",sceneid);
	
	if(mcuid >0)
		printn += sprintf(sql+printn,"AND _mcuid=%d ",mcuid);

	if(starttime!= NULL)
		printn += sprintf(sql+printn,"AND _time>='%s' ",starttime);
	
	if(endtime != NULL)
		printn += sprintf(sql+printn,"AND _time<'%s' ",endtime);

	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt == 0)
	{
		scenestatus_st status;
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while(dbinst->m_row = mysql_fetch_row(dbinst->m_res))
		{
			if(dbinst->m_row[0])
				status.sceneid = atoi(dbinst->m_row[0]);							

			if(dbinst->m_row[1])
				status.mcuid= atoi(dbinst->m_row[1]);							

			if(dbinst->m_row[2])
				status.type = atoi(dbinst->m_row[2]);							

			if(dbinst->m_row[3])
				status.battery = atoi(dbinst->m_row[3]);		

			if(dbinst->m_row[4])
				status.mcustate = atoi(dbinst->m_row[4]);		

			if(dbinst->m_row[5])
				status.sensorstate = atoi(dbinst->m_row[5]);		

			if(dbinst->m_row[6])
				status.sensortype = atoi(dbinst->m_row[6]);		

			if(dbinst->m_row[7])
				strcpy(status.time,dbinst->m_row[7]);		

			scenestatuses_add(statuses,&status);
		}
		mysql_free_result(dbinst->m_res);
	}else{
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scenestatus . query error :%s",mysql_error(dbinst->m_con));		
	}

	return rt;
}

int db_load_scenes(mysqlquery_t dbinst,int sceneid,sceneinfos_t rslt)
{
	char sql[512] = {0};
	int  rt = 1;
	int printn = 0;

	if(dbinst == NULL || sceneid <0 || rslt == NULL)
		return rt;

	sceneinfos_clear(rslt);

	printn += sprintf(sql+printn,"SELECT _id,_name,_template FROM tbScene ",sceneid);

	if(sceneid > 0)
		printn += sprintf(sql+printn,"WHERE _id=%d ",sceneid);	

	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt == 0)
	{
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while(dbinst->m_row = mysql_fetch_row(dbinst->m_res))
		{
			sceneinfo_t info = sceneinfo_new();

			if(dbinst->m_row[0])
				info->sceneid = atoi(dbinst->m_row[0]);							

			if(dbinst->m_row[1])
				info->scenename = _strdup(dbinst->m_row[1]);							

			if(dbinst->m_row[2])
				info->templ = _strdup(dbinst->m_row[2]);											

			sceneinfos_add(rslt,info);
		}
		mysql_free_result(dbinst->m_res);
	}else{
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s",mysql_error(dbinst->m_con));		
	}

	return rt;
}

int db_load_visitor_activity(mysqlquery_t dbinst,int cardid,visitrslt_t rslt,int rsltcap)
{
	char sql[512] = {0};
	int  rt = 1;
	int printn = 0;
	int count = 0;

	if(dbinst == NULL || cardid <0 || rslt == NULL)
		return rt;

	printn += sprintf(sql+printn,"SELECT DISTINCT Vist._sceneid,Scene._name, Vist._time FROM tbVisitorActivity AS Vist,tbScene AS Scene WHERE _userid=%d AND Vist._sceneid=Scene._id",cardid);

	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt == 0)
	{
		count = 0;
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while((dbinst->m_row = mysql_fetch_row(dbinst->m_res)) != NULL && count < rsltcap)
		{
			if(dbinst->m_row[0]){
				rslt[count].id = atoi(dbinst->m_row[0]);	
				strncpy(rslt[count].name, dbinst->m_row[1],sizeof(rslt[count].name)-1) ;	
				strncpy(rslt[count].time, dbinst->m_row[2],sizeof(rslt[count].time)-1) ;	
				count++;
			}
		}
		mysql_free_result(dbinst->m_res);
	}else{
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s",mysql_error(dbinst->m_con));		
	}

	return count;
}

int db_clear_visitor_activtiy(mysqlquery_t dbinst,int cardid)
{
	char sql[512] = {0};
	int  rt = 1;
	int printn = 0;

	if(dbinst == NULL || cardid <0)
		return rt;

	printn += sprintf_s(sql + printn, sizeof(sql) - printn -1, "DELETE FROM tbVisitorActivity where _userid=%d", cardid);

	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt != 0){
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s",mysql_error(dbinst->m_con));			
	}

	return rt;
}

int db_load_statistic_scene_scores(mysqlquery_t dbinst,int sceneid,scoredeploy_st*out)
{
	char sql[512] = {0};
	int  rt = 1;
	int printn = 0;

	if(dbinst == NULL || sceneid <0 || out == NULL)
		return rt;

	printn += sprintf_s(sql + printn, sizeof(sql) - printn -1, "SELECT _sceneid,_json_score,_time FROM tbStatisticSceneScore where _sceneid=%d", sceneid);

	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt == 0)
	{
		int count = 0;
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while((dbinst->m_row = mysql_fetch_row(dbinst->m_res)) != NULL)
		{
			out->sceneid = sceneid;

			strncpy_s(out->buffer, sizeof(out->buffer), dbinst->m_row[1], sizeof(out->buffer));
		}
		mysql_free_result(dbinst->m_res);
	}else{
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s",mysql_error(dbinst->m_con));		
	}

	return rt;
}

int db_load_statistic_scene_visitors(mysqlquery_t dbinst,int sceneid,agedeploy_st* out)
{
	char sql[512] = {0};
	int  rt = 1;
	int printn = 0;

	if(dbinst == NULL || sceneid <0 || out == NULL)
		return rt;

	printn += sprintf_s(sql + printn, sizeof(sql) - printn - 1,"SELECT _sceneid,_json_visitor,_time FROM tbStatisticSceneVisitor where _sceneid=%d", sceneid);

	rt = mysql_real_query(dbinst->m_con,sql,strlen(sql));

	if(rt == 0)
	{
		int count = 0;
		dbinst->m_res = mysql_store_result(dbinst->m_con);
		while((dbinst->m_row = mysql_fetch_row(dbinst->m_res)) != NULL)
		{
			out->sceneid = sceneid;

			strncpy_s(out->buffer, sizeof(sql) - printn - 1, dbinst->m_row[1], sizeof(out->buffer));
		}
		mysql_free_result(dbinst->m_res);
	}else{
		if(dbinst->_cb)
			dbinst->_cb(dbinst,eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s",mysql_error(dbinst->m_con));		
	}

	return rt;
}


//////////////////////////////////////////////////////////////////////////
#define _unsettled_orders_fields_cnt 11
#define _unsettled_orders_fields       "ordertime,orderno,username,age,identitycard,phone,workunit,email,team_name,group_name,leader"
#define _unsettled_orders_fields_para  "'%s','%s','%s',%d,'%s','%s','%s','%s','%s','%s','%s'"
#define _unsettled_orders_values(_st_) _st_->orderdate,_st_->orderno,_st_->username,_st_->age,_st_->cardid,_st_->telephone,_st_->workunit,_st_->email,_st_->team,_st_->group,_st_->teamleader

int db_query_unsettled_orders(mysqlquery_t dbinst, char* where, unsettled_order_t * outorders, int * count)
{
	char sql[1024] = { 0 };
	int  printn = 0;

	*outorders = NULL;
	*count = 0;

	if (dbinst == NULL || where == NULL || strlen(where) == 0) return _QFAILE;

	printn += sprintf_s(sql + printn, sizeof(sql) - printn, \
		               "SELECT ("_unsettled_orders_fields")\
					    FROM tbUnsettledOrder WHERE %s", where);
	
	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		dbinst->m_res = mysql_store_result(dbinst->m_con);

		*count = mysql_num_rows(dbinst->m_res);
		*outorders = calloc(*count, sizeof(unsettled_order_st));

		int ndx = 0;
		while ((dbinst->m_row = mysql_fetch_row(dbinst->m_res)) != NULL)
		{
			if (dbinst->m_row[0]) strcpy_s((*outorders)[ndx].orderdate, sizeof((*outorders)[ndx].orderdate),	dbinst->m_row[0]);
			if (dbinst->m_row[1]) strcpy_s((*outorders)[ndx].orderno,   sizeof((*outorders)[ndx].orderno),		dbinst->m_row[1]);
			if (dbinst->m_row[2]) strcpy_s((*outorders)[ndx].username,  sizeof((*outorders)[ndx].username),		dbinst->m_row[2]);
			if (dbinst->m_row[3]) (*outorders)[ndx].age,s_atoi(dbinst->m_row[3]);
			if (dbinst->m_row[4]) strcpy_s((*outorders)[ndx].cardid,    sizeof((*outorders)[ndx].cardid),		dbinst->m_row[4]);
			if (dbinst->m_row[5]) strcpy_s((*outorders)[ndx].telephone, sizeof((*outorders)[ndx].telephone),	dbinst->m_row[5]);
			if (dbinst->m_row[6]) strcpy_s((*outorders)[ndx].workunit,  sizeof((*outorders)[ndx].workunit),		dbinst->m_row[6]);
			if (dbinst->m_row[7]) strcpy_s((*outorders)[ndx].email,     sizeof((*outorders)[ndx].email),		dbinst->m_row[7]);
			if (dbinst->m_row[8]) strcpy_s((*outorders)[ndx].team,      sizeof((*outorders)[ndx].team),			dbinst->m_row[8]);
			if (dbinst->m_row[9]) strcpy_s((*outorders)[ndx].group,     sizeof((*outorders)[ndx].group),		dbinst->m_row[9]);
			if (dbinst->m_row[10]) strcpy_s((*outorders)[ndx].teamleader, sizeof((*outorders)[ndx].teamleader), dbinst->m_row[10]);
			ndx++;
		}
		mysql_free_result(dbinst->m_res);

		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

int db_save_unsettled_orders(mysqlquery_t dbinst, unsettled_order_t orders, int count)
{
	membuff_t sql = membuff_new(8*1024);
	membuff_t mbuffdata = membuff_new(4*1024);

	if (dbinst == NULL || orders == NULL || count < 1) return _QFAILE;
	
	for (int i = 0; i < count; i++)
		membuff_add_printf(mbuffdata, "("_unsettled_orders_fields_para"),", _unsettled_orders_values((&orders[i])));
	membuff_trim(mbuffdata, ",");
	membuff_addstr(mbuffdata, ";", 2);

	membuff_add_printf(sql, "INSERT INTO tbUnsettledOrder("_unsettled_orders_fields") VALUES %s", membuff_data(mbuffdata));

	if (mysql_real_query(dbinst->m_con, membuff_data(sql), membuff_len(sql)) == 0)
	{
		membuff_free(sql);
		membuff_free(mbuffdata);
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		membuff_free(sql);
		membuff_free(mbuffdata);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));

		return _QFAILE;
	}
}

int db_delete_unsettled_orders(mysqlquery_t dbinst, char* orderno, char* username)
{
	char sql[512] = { 0 };
	int  printn = 0;

	if (dbinst  == NULL || (orderno == NULL && username == NULL)) return _QFAILE;
	if (orderno == NULL)  return _QFAILE;

	if (username == NULL){
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "DELETE FROM tbUnsettledOrder"\
			"where orderno=%s");
	}
	else
	{
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "DELETE FROM tbUnsettledOrder"\
			" where orderno=%s AND username=%s", orderno,username);
	}


	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

int db_modify_unsettled_orders(mysqlquery_t dbinst, char* where, char* set)
{
	char sql[512] = { 0 };
	int  printn = 0;

	if (dbinst == NULL || where == NULL || set == NULL || strlen(where) == 0 || strlen(set) == NULL) return _QFAILE;

	printn += sprintf_s(sql + printn, sizeof(sql) - printn, "UPDATE tbUnsettledOrder SET %s  WHERE %s", set, where);

	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

//////////////////////////////////////////////////////////////////////////
#define _settled_orders_fields_cnt   13
#define _settled_orders_fields       "ordertime,orderno,username,age,identitycard,phone,workunit,email,team_name,group_name,leader,oper,opertime"
#define _settled_orders_fields_para  "'%s','%s','%s',%d,'%s','%s','%s','%s','%s','%s','%s','%s','%s'"
#define _settled_orders_values(_st_) _st_->orderdate,_st_->orderno,_st_->username,_st_->age,_st_->cardid,_st_->telephone,_st_->workunit,_st_->email,_st_->team,_st_->group,_st_->teamleader,_st_->operator,_st_->opertime

int db_query_settled_orders(mysqlquery_t dbinst, char* where, settled_order_t * outorders, int * count)
{
	char sql[1024] = { 0 };
	int  printn = 0;

	*outorders = NULL;
	*count = 0;

	if (dbinst == NULL || where == NULL || strlen(where) == 0) return _QFAILE;

	printn += sprintf_s(sql + printn, sizeof(sql) - printn, \
		"SELECT ("_settled_orders_fields") FROM tbSettledOrder WHERE %s", where);

	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		dbinst->m_res = mysql_store_result(dbinst->m_con);

		*count = mysql_num_rows(dbinst->m_res);
		*outorders = calloc(*count, sizeof(settled_order_st));
		
		int ndx = 0;
		while ((dbinst->m_row = mysql_fetch_row(dbinst->m_res)) != NULL)
		{
			if (dbinst->m_row[0]) strcpy_s((*outorders)[ndx].orderdate, sizeof((*outorders)[ndx].orderdate), dbinst->m_row[0]);
			if (dbinst->m_row[1]) strcpy_s((*outorders)[ndx].orderno,   sizeof((*outorders)[ndx].orderno), dbinst->m_row[1]);
			if (dbinst->m_row[2]) strcpy_s((*outorders)[ndx].username,  sizeof((*outorders)[ndx].username), dbinst->m_row[2]);
			if (dbinst->m_row[3]) (*outorders)[ndx].age, s_atoi(dbinst->m_row[3]);
			if (dbinst->m_row[4]) strcpy_s((*outorders)[ndx].cardid,    sizeof((*outorders)[ndx].cardid), dbinst->m_row[4]);
			if (dbinst->m_row[5]) strcpy_s((*outorders)[ndx].telephone, sizeof((*outorders)[ndx].telephone), dbinst->m_row[5]);
			if (dbinst->m_row[6]) strcpy_s((*outorders)[ndx].workunit,  sizeof((*outorders)[ndx].workunit), dbinst->m_row[6]);
			if (dbinst->m_row[7]) strcpy_s((*outorders)[ndx].email,     sizeof((*outorders)[ndx].email), dbinst->m_row[7]);
			if (dbinst->m_row[8]) strcpy_s((*outorders)[ndx].team,      sizeof((*outorders)[ndx].team), dbinst->m_row[8]);
			if (dbinst->m_row[9]) strcpy_s((*outorders)[ndx].group,     sizeof((*outorders)[ndx].group), dbinst->m_row[9]);
			if (dbinst->m_row[10]) strcpy_s((*outorders)[ndx].teamleader, sizeof((*outorders)[ndx].teamleader), dbinst->m_row[10]);
			if (dbinst->m_row[11]) strcpy_s((*outorders)[ndx].operator, sizeof((*outorders)[ndx].group), dbinst->m_row[9]);
			if (dbinst->m_row[12]) strcpy_s((*outorders)[ndx].opertime, sizeof((*outorders)[ndx].teamleader), dbinst->m_row[10]);
			ndx++;
		}
		mysql_free_result(dbinst->m_res);

		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

int db_save_settled_orders(mysqlquery_t dbinst, settled_order_t orders, int count)
{
	membuff_t sql = membuff_new(8 * 1024);
	membuff_t mbuffdata = membuff_new(4 * 1024);

	if (dbinst == NULL || orders == NULL || count < 1) return _QFAILE;

	for (int i = 0; i < count; i++)
		membuff_add_printf(mbuffdata, "("_settled_orders_fields_para"),", _settled_orders_values((&orders[i])));
	membuff_trim(mbuffdata, ",");
	membuff_addstr(mbuffdata, ";", 2);

	membuff_add_printf(sql, "INSERT INTO tbSettledOrder("_settled_orders_fields") VALUES %s", membuff_data(mbuffdata));

	if (mysql_real_query(dbinst->m_con, membuff_data(sql), membuff_len(sql)) == 0)
	{
		membuff_free(sql);
		membuff_free(mbuffdata);
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		membuff_free(sql);
		membuff_free(mbuffdata);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));

		return _QFAILE;
	}
}

int db_delete_settled_orders(mysqlquery_t dbinst, char* orderno, char* username)
{
	char sql[512] = { 0 };
	int  printn = 0;

	if (dbinst == NULL || (orderno == NULL && username == NULL)) return _QFAILE;
	if (orderno == NULL)  return _QFAILE;

	if (username == NULL){
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "DELETE FROM tbSettledOrder"\
			"where orderno=%s");
	}
	else
	{
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "DELETE FROM tbSettledOrder"\
			" where orderno=%s AND username=%s", orderno, username);
	}


	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

int db_modify_settled_orders(mysqlquery_t dbinst, char* where, char* set)
{
	char sql[512] = { 0 };
	int  printn = 0;

	if (dbinst == NULL || where == NULL || set == NULL || strlen(where) == 0 || strlen(set) == NULL) return _QFAILE;

	printn += sprintf_s(sql + printn, sizeof(sql) - printn, "UPDATE tbSettledOrder SET %s  WHERE %s", set, where);

	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

int db_save_card(mysqlquery_t dbinst, char * cardid, char* cardsn)
{
	char sql[512] = { 0 };
	int  printn   = 0;
	char timestr[32] = {0};

	if (dbinst == NULL || (cardid == NULL && cardsn == NULL)) return _QFAILE;
	if (cardsn == NULL) cardsn = "";
	if (cardid == NULL) cardid = "";

	current_time_str(timestr);
	printn += sprintf_s(sql + printn, sizeof(sql) - printn, "INSERT INTO tbCard(cardid,cardsn,ctime) VALUES(%s,%s,%s)", cardid, cardsn, timestr);

	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}

	
}

int db_del_card(mysqlquery_t dbinst, char * cardid, char* cardsn)
{
	char sql[512] = { 0 };
	int  printn = 0;

	if (dbinst == NULL) return _QFAILE;

	if (cardsn == NULL && cardid == NULL)
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "DELETE FROM tbCard");
	else if (cardid == NULL)
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "DELETE FROM tbCard WHERE cardsn=%s", cardsn);
	else if (cardsn == NULL)
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "DELETE FROM tbCard WHERE cardid=%s", cardid);
	else
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "DELETE FROM tbCard WHERE cardid=%s AND cardsn=%s", cardid, cardsn);
	

	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

int db_mod_card_byid(mysqlquery_t dbinst, char * cardid, char* cardsn)
{
	char sql[512] = { 0 };
	int  printn = 0;

	if (dbinst == NULL || cardid == NULL) return _QFAILE;
	if (cardsn == NULL)   cardsn = "";

	printn += sprintf_s(sql + printn, sizeof(sql) - printn,"UPDATE tbCard SET cardsn=%s WHERE cardid=%s", cardsn, cardid);

	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

int db_mod_card_bysn(mysqlquery_t dbinst, char * cardid, char* cardsn)
{
	char sql[512] = { 0 };
	int  printn = 0;

	if (dbinst == NULL || cardsn == NULL) return _QFAILE;
	if (cardid == NULL)   cardid = "";

	printn += sprintf_s(sql + printn, sizeof(sql) - printn, "UPDATE tbCard SET cardid=%s WHERE cardsn=%s", cardid, cardsn);

	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}

int db_query_card(mysqlquery_t dbinst, char*cardid, char* cardsn, card_t* cards, int *count)
{
	char sql[512] = { 0 };
	int  printn = 0;

	*cards = NULL;
	*count = 0;

	if (dbinst == NULL || (cardsn == NULL && cardid == NULL)) return _QFAILE;

	if (cardsn == NULL)
		printn += sprintf_s(sql + printn, sizeof(sql) - printn,"SELECT (cardid,cardsn,uptime,ctime) FROM tbCard WHERE cardid=%s", cardid);
	else if (cardid == NULL)
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "SELECT (cardid,cardsn,uptime,ctime) FROM tbCard WHERE cardsn=%s", cardsn);
	else
		printn += sprintf_s(sql + printn, sizeof(sql) - printn, "SELECT (cardid,cardsn,uptime,ctime) FROM tbCard WHERE cardsn=%s AND cardid=%s", cardsn, cardid);

	if (mysql_real_query(dbinst->m_con, sql, strlen(sql)) == 0)
	{
		dbinst->m_res = mysql_store_result(dbinst->m_con);

		*count = mysql_num_rows(dbinst->m_res);
		*cards = calloc(*count, sizeof(card_st));

		int ndx = 0;
		while ((dbinst->m_row = mysql_fetch_row(dbinst->m_res)) != NULL)
		{
			if (dbinst->m_row[0]) strcpy_s((*cards)[ndx].cardid, sizeof((*cards)[ndx].cardid), dbinst->m_row[0]);
			if (dbinst->m_row[1]) strcpy_s((*cards)[ndx].cardsn, sizeof((*cards)[ndx].cardsn), dbinst->m_row[1]);
			if (dbinst->m_row[2]) strcpy_s((*cards)[ndx].utime,  sizeof((*cards)[ndx].utime),  dbinst->m_row[2]);
			if (dbinst->m_row[3]) strcpy_s((*cards)[ndx].ctime,  sizeof((*cards)[ndx].ctime),  dbinst->m_row[3]);
			ndx++;
		}
		mysql_free_result(dbinst->m_res);

		return _QOK;
	}
	else{
		if (dbinst->_cb)
			dbinst->_cb(dbinst, eSqlQueryerr_errorping);

		printf("db_load_scenes . query error :%s", mysql_error(dbinst->m_con));
		return _QFAILE;
	}
}