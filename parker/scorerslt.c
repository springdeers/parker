#include "scorerslt.h"
#include <stdlib.h>
#include <string.h>
#include <string.h>
//#include "cJSON.h"
#include "json/cJSON.h"
#include "mysqldb.h"
#include "appconfig.h"

extern config_st g_conf;
extern mysqlquery_t sqlobj_venue_db;


void scores_clear(scores_t con)
{
	if(con){
		memset(con,0,sizeof(scores_st));
	}
}

int  scores_add(scores_t con,score_t score)
{
	if(con->scorenum < _maxscore_num)
		con->score[con->scorenum++] = *score;

	return con->scorenum;
}

void scores_freeitems(scores_t con)
{
	int i = 0;

	if(con == NULL) return;

	for(; i < con->scorenum ; i++){
		free(con->score[i].json);
		free(con->score[i].scenename);
	}
}

static int compare_score(const void* L,const void* R)
{
	score_t s1,s2;
	if(L==NULL ||R==NULL) return 0;
	
	s1 = (score_t)L;
	s2 = (score_t)R;
	
	return s1->sceneid<s2->sceneid;
}

void scores_shrink(scores_t scores)
{	
	int count = 0;
	int i = 0;
	if(scores == NULL) return ;

	count = scores->scorenum;
	qsort(scores->score,scores->scorenum,sizeof(scores->score[0]),compare_score);	
	
	for(; i < scores->scorenum-1 ; i++){
		if(scores->score[i].sceneid == scores->score[i+1].sceneid){
			if(scores->score[i].fscore <= scores->score[i+1].fscore){
				int remain = 0;
				free(scores->score[i].json);
				free(scores->score[i].scenename);
				remain = scores->scorenum-i-1;
				if(remain<0) remain = 0;
				memcpy(&scores->score[i],&scores->score[i+1],remain*sizeof(score_st));
				scores->scorenum--;
			}else{
				int remain = 0;
				free(scores->score[i+1].json);
				free(scores->score[i+1].scenename);
				remain = scores->scorenum-i-2;
				if(remain<0) remain = 0;
				memcpy(&scores->score[i+1],&scores->score[i+2],remain*sizeof(score_st));
				scores->scorenum--;
			}
			i--;
		}
	}
}
char* replace_score_suggest(score_t score,scores_scs_t score_scs){

	cJSON *cj_root,*cj_comment,*cj_class,*cj_score,*cj_scene;
	float f_score;
	int   i_scene,i;
	score_ran_t score_tan;
	char *r_class,*r_sugg,*out;

	cj_root = cJSON_Parse(score->json);
	if(cj_root){
		cj_scene = cJSON_GetObjectItem(cj_root,"scene");
		cj_score = cJSON_GetObjectItem(cj_root,"score");
		i_scene = atoi(cj_scene->valuestring);
		f_score = atof(cj_score->valuestring);
		r_class = "无";
		r_sugg = "无";
		for(i=0;i<score_scs->scorenum;i++){
			score_tan = &score_scs->score[i];
			if(score_tan->sceneid==i_scene){
				if(f_score>=score_tan->s1 && f_score<=score_tan->s2){
					r_class = score_tan->c_class;
					r_sugg = score_tan->c_sug;
					break;
				}
			}
		}
		cj_comment = cJSON_CreateString(r_sugg);
		cj_class = cJSON_CreateString(r_class);
		cJSON_ReplaceItemInObject(cj_root,"comment",cj_comment);
		cJSON_ReplaceItemInObject(cj_root,"class",cj_class);
	}
	out=cJSON_Print(cj_root);
	if(cj_root){
		cJSON_Delete(cj_root);
	}
	return out;
}

void scores_scs_free(scores_scs_t scores)
{
	int i = 0;
	if(scores == NULL) return;
	for(; i < scores->scorenum ; i++){
		free(scores->score[i].c_class);
		free(scores->score[i].c_sug);
	}
}


// 由venue_db返回的各个场景的单独成绩，综合运算得出最终的总成绩

/******************************************************************************
/*  核心算法描述：
/*  总成绩 = 学分点*（学分点所占百分比） + 绩点*（绩点所占百分比）
/*  例如   = 学分点*20% + 绩点*80%
/*  其中：
/*  学分点 = (所获学分总和 / 所有场景学分总和) * 100
/*  绩点   = (场景1成绩*场景1学分 + …… + 场景N成绩*场景N学分) / 所获学分总和
*******************************************************************************/
int get_finalscore(scores_t scores, finalscore_t finalscore)
{
	int ret = -1;

	int i = 0;
	int credit = 0;
	int obtained_credits = 0;				// 所获总学分
	float fscore = 0;
	float obtaind_fscore = 0;				// 成绩加权和
	
	for (i = 0; i < scores->scorenum; i++)
	{
		credit = scores->score[i].credit;								// 单个场景的学分
		fscore = scores->score[i].fscore;								// 单个场景的成绩
		fscore = fscore * (float)credit;								// 单个场景的成绩*学分

		obtained_credits += credit;										// 所得学分累加
		obtaind_fscore += fscore;										// 成绩加权和
	}


	// 1. credit_point

	finalscore->credit_point = (float)obtained_credits*1.0f / (float)scores->totalcredit*1.0f * 100;

	// 2. grade_point

	finalscore->grade_point = obtaind_fscore / obtained_credits;		

	// 3. finalscore
	finalscore->finalscore = finalscore->credit_point * g_conf.credit_proportion + finalscore->grade_point * g_conf.grade_proportion;

	// 4. scoerclass and suggestion
	if (finalscore->finalscore > 90  && finalscore->finalscore <= 100)
	{
		strcpy_s(finalscore->scoreclass, strlen(g_conf.score_class_A) + 1, g_conf.score_class_A);
		strcpy_s(finalscore->suggestion, strlen(g_conf.score_suggestion_A) + 1, g_conf.score_suggestion_A);
	}
	else if (finalscore->finalscore > 70 && finalscore->finalscore <= 90)
	{
		strcpy_s(finalscore->scoreclass, strlen(g_conf.score_class_B) + 1, g_conf.score_class_B);
		strcpy_s(finalscore->suggestion, strlen(g_conf.score_suggestion_B) + 1, g_conf.score_suggestion_B);
	}
	else if (finalscore->finalscore > 0)
	{
		strcpy_s(finalscore->scoreclass, strlen(g_conf.score_class_C) + 1, g_conf.score_class_C);
		strcpy_s(finalscore->suggestion, strlen(g_conf.score_suggestion_C) + 1, g_conf.score_suggestion_C);
	}

	// 5. remain_scene_cnt and remain_scenes
	finalscore->rmnscene_cnt = scores->totalscenes - scores->scorenum;
	memcpy(finalscore->rmnscenes, scores->remains, sizeof(scores->remains));

	return ret;
}