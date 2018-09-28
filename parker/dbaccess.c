#include "dbaccess.h"
#include <stdio.h>

extern mysqlquery_t sqlobj_venue_db;


int db_query_credits(mysqlquery_t dbinst, int* cnt, int *credits)
{
	char sql[512] = { 0 };
	int  rt = -1;

	if (dbinst == NULL)
		return rt;

	sprintf_s(sql, sizof(sql) - 1, "SELECT COUNT(*),SUM(credit) FROM tbScene");
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
int db_query_travelrname(mysqlquery_t dbinst, int cardid, char **name)
{
	char sql[512] = { 0 };
	int  rt = -1;

	if (dbinst == NULL)
		return rt;

	sprintf(sql, sizeof(sql) - 1, "SELECT nickame FROM tbtraveler WHERE userid=%d", cardid);
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