#ifndef _CONFIG_H__
#define _CONFIG_H__

typedef struct _config_st{
	int   svrport;					//�������˿ڡ�
	char* venue_db_ip;
	int   venue_db_port;
	char* venue_db_name;			
	char* venue_db_username; 
	char* venue_db_userpwd; 
	char* userinfo_db_ip;
	int   userinfo_db_port;
	char* userinfo_db_name;   
	char* userinfo_db_username; 
	char* userinfo_db_userpwd;
	float credit_proportion;		// ѧ�ֵ���ռ����
	float grade_proportion;			// ������ռ����
	char *score_class_A;
	char *score_class_B;
	char *score_class_C;
	char *score_suggestion_A;
	char *score_suggestion_B;
	char *score_suggestion_C;
	int   score_scs_refresh;//��λ��

}config_st,*config_t;

#endif