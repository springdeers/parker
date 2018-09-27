#ifndef _SCORERSLT_H__
#define _SCORERSLT_H__
	
typedef struct _score_st{
	int   sceneid;
	char* scenename;
	int   credit;			// ���ѧ���ֶ�
	int cardid;
	float fscore;
	int state;
	int secondcost;
	char* json;	
}score_st,*score_t;

#define _maxscore_num (128)

typedef struct _scores_st{
	char name[30];					// ����������
	int totalcredit;				// ���г�����ѧ��
	int totalscenes;				// ���г�������
	int scorenum;					// ����������óɼ�������
	score_st score[1024];			// ������ĸ��������ɼ�
	int remain_cnt;					// ����δ���鳡������
	char remains[2048];				// ����δ���鳡�������б�
}scores_st,*scores_t;

typedef struct _finalscore_st{
	float	finalscore;			// ���հٷ��Ƶķ���
	float	credit_point;		// ѧ�ֵ�
	float	grade_point;		// ����
	char	scoreclass[10];		// �ɼ�����
	char	suggestion[1024];	// ����
	int		rmnscene_cnt;		// δ����ĳ�������
	char	rmnscenes[2048];	// δ����ĳ��������б��磺 "������ˮ������������VR��ʻ"	
}finalscore_st, *finalscore_t;

//�÷����ö���
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