#include "jsonconf.h"
#include <assert.h>
#include "json/cJSON.h"
#include <Windows.h>
#include <string.h>
#include <stdio.h>

#define FALSE    0
#define TRUE     1
#define NULL     ((void*)0)

BOOL setup_config(char* jsonfile,config_t cfg){
	BOOL  bRtn = FALSE;
	char  *out;
	cJSON *json;
	char  imgpath[256];
	char* pos = 0;
	FILE  *file = NULL;
	int   filelen = 0;

	assert(cfg);

	if(cfg == NULL) return FALSE;
	
	memset(cfg,0,sizeof(config_st));

	GetModuleFileName(NULL,imgpath,sizeof(imgpath));
	pos = strrchr(imgpath,'\\');
	*(++pos) = '\0';
	strcat(imgpath,jsonfile);

	if((file = fopen(imgpath,"rb")) == NULL){
		printf("打开配置文件失败!\n");
		return FALSE;
	}

	fseek(file,0,SEEK_END);
	filelen = ftell(file);
	out = calloc(filelen+1,1);

	fseek(file,0,0);
	fread(out,1,filelen,file);
	json = cJSON_Parse(out);

	while(1)
	{
		cJSON* item = cJSON_GetObjectItem(json,"svrport");
		if(item == NULL) break;
		cfg->svrport = atoi(item->valuestring);

		item = cJSON_GetObjectItem(json,"db_ip");
		if(item == NULL) break;
		cfg->db_ip = _strdup(item->valuestring);

		item = cJSON_GetObjectItem(json,"db_port");
		if(item == NULL) break;
		cfg->db_port = atoi(item->valuestring);

		item = cJSON_GetObjectItem(json,"db_name");
		if(item == NULL) break;
		cfg->db_name = _strdup(item->valuestring);

		item = cJSON_GetObjectItem(json,"db_username");
		if(item == NULL) break;
		cfg->db_username = _strdup(item->valuestring);
				
		item = cJSON_GetObjectItem(json,"db_userpwd");
		if(item == NULL) break;
		cfg->db_userpwd = _strdup(item->valuestring);

		item = cJSON_GetObjectItem(json, "score_scs_refresh");
		if (item == NULL) break;
		cfg->score_scs_refresh = atoi(item->valuestring);

		item = cJSON_GetObjectItem(json, "credit_proportion");
		if (item == NULL) break;
		cfg->credit_proportion = atof(item->valuestring);

		item = cJSON_GetObjectItem(json, "grade_proportion");
		if (item == NULL) break;
		cfg->grade_proportion = atof(item->valuestring);

		item = cJSON_GetObjectItem(json, "score_class_A");
		if (item == NULL) break;
		cfg->score_class_A = _strdup(item->valuestring);

		item = cJSON_GetObjectItem(json, "score_class_B");
		if (item == NULL) break;
		cfg->score_class_B = _strdup(item->valuestring);

		item = cJSON_GetObjectItem(json, "score_class_C");
		if (item == NULL) break;
		cfg->score_class_C = _strdup(item->valuestring);

		item = cJSON_GetObjectItem(json, "score_suggestion_A");
		if (item == NULL) break;
		cfg->score_suggestion_A = _strdup(item->valuestring);

		item = cJSON_GetObjectItem(json, "score_suggestion_B");
		if (item == NULL) break;
		cfg->score_suggestion_B = _strdup(item->valuestring);

		item = cJSON_GetObjectItem(json, "score_suggestion_C");
		if (item == NULL) break;
		cfg->score_suggestion_C = _strdup(item->valuestring);

		break;
	}

	if (json) cJSON_Delete(json);

	return TRUE;
}
