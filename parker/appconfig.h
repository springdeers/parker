#ifndef _CONFIG_H__
#define _CONFIG_H__

typedef struct _config_st{
	int   svrport;  //服务器端口。
	char* db_ip;
	int   db_port;//
	char* db_name;   //安装包程序名称。用于向服务器获取的文件名。	
	char* db_username; //本地已经获取的补丁包版本号。
	char* db_userpwd; //本地已经获取的补丁包版本号。
	float credit_proportion;	// 学分点所占比例
	float grade_proportion;	// 绩点所占比例
	char *score_class_A;
	char *score_class_B;
	char *score_class_C;
	char *score_suggestion_A;
	char *score_suggestion_B;
	char *score_suggestion_C;
	int   score_scs_refresh;//单位秒

}config_st,*config_t;

#endif