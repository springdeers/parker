#ifndef _CONFIG_H__
#define _CONFIG_H__

typedef struct _config_st{
	int   svrport;  //�������˿ڡ�
	char* db_ip;
	int   db_port;//
	char* db_name;   //��װ���������ơ��������������ȡ���ļ�����	
	char* db_username; //�����Ѿ���ȡ�Ĳ������汾�š�
	char* db_userpwd; //�����Ѿ���ȡ�Ĳ������汾�š�
	float credit_proportion;	// ѧ�ֵ���ռ����
	float grade_proportion;	// ������ռ����
	char *score_class_A;
	char *score_class_B;
	char *score_class_C;
	char *score_suggestion_A;
	char *score_suggestion_B;
	char *score_suggestion_C;
	int   score_scs_refresh;//��λ��

}config_st,*config_t;

#endif