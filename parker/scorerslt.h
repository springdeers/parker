#ifndef _SCORERSLT_H__
#define _SCORERSLT_H__
	
typedef struct _score_st{
	int   sceneid;
	char* scenename;
	int   credit;			// 添加学分字段
	int cardid;
	float fscore;
	int state;
	int secondcost;
	char* json;	
}score_st,*score_t;

#define _maxscore_num (128)

typedef struct _scores_st{
	char name[30];					// 体验者姓名
	int totalcredit;				// 所有场景总学分
	int totalscenes;				// 所有场景总数
	int scorenum;					// 体验过后所得成绩的数量
	score_st score[1024];			// 体验过的各个场景成绩
	int remain_cnt;					// 所有未体验场景总数
	char remains[2048];				// 所有未体验场景名称列表
}scores_st,*scores_t;

typedef struct _finalscore_st{
	float	finalscore;			// 最终百分制的分数
	float	credit_point;		// 学分点
	float	grade_point;		// 绩点
	char	scoreclass[10];		// 成绩档次
	char	suggestion[1024];	// 建议
	int		rmnscene_cnt;		// 未体验的场景数量
	char	rmnscenes[2048];	// 未体验的场景名称列表，如： "汽车落水、烟雾逃生、VR驾驶"	
}finalscore_st, *finalscore_t;

//得分设置对象
#define _maxscore_scs_num (1024)
typedef struct _score_ran_st{
	int sceneid;
	float s1;
	float s2;
	char* c_class;
	char* c_sug;
}score_ran_st,*score_ran_t;

typedef struct _scores_scs_st{
	int scorenum;
	score_ran_st score[_maxscore_scs_num];
}scores_scs_st,*scores_scs_t;


void scores_clear(scores_t);
int  scores_add(scores_t con,score_t score);
void scores_freeitems(scores_t);
void scores_shrink(scores_t);
char* replace_score_suggest(score_t score,scores_scs_t score_scs);
void scores_scs_free(scores_scs_t scores);

// ywy 2018-9-11
int get_finalscore(scores_t scores, finalscore_t finalscore);

#endif