// parker.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "event2/event.h"
#include "event2/keyvalq_struct.h"
#include "event2/buffer.h"
#include "event2/http.h"
#include "dbaccess.h"
#include "route.h"
#include <winsock2.h>
#include "jsonconf.h"
#include "cutil/util.h"
#include <time.h>
#include "route.h"
#include "orderquery.h"
#include "userquery.h"

mysqlquery_t mysqlconn = NULL;
config_st    g_conf;
log_t        g_log;

typedef struct _tick_st{
	TIMEVAL tv;
	struct event* timer;
}tick_st,*tick_t;

int  start_httpd(const char* ip, int port, void (*cb)(struct evhttp_request *, void *), void *arg);
void httpd_callback(struct evhttp_request* req, void* arg);
void mysql_querycallback(void* conn,int code);
void ontimer(evutil_socket_t listener,short event,void* arg);

void init_winsocklib()
{
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

int _tmain(int argc, _TCHAR* argv[])
{	
	BOOL bconf = setup_config("parker.json",&g_conf);
	if (!bconf) {
		printf("err : load ./parker.json file failed.  exit!\n");
		exit(1);
	}
	
	g_log = log_new(log_FILE,"./log.txt",NULL);
	if(!g_log){
		g_log = log_new(log_STDOUT,NULL,NULL);
		printf("open logfile failed. log goto stdout instead.\n");
	}

	init_winsocklib();
	
	mysqlconn = mysqldb_connect_init(g_conf.db_ip,g_conf.db_port,g_conf.db_username,g_conf.db_userpwd,g_conf.db_name,mysql_querycallback);    	
	router_setup();
	//启动服务在地址 127.0.0.1:9000 上
	start_httpd("0.0.0.0",g_conf.svrport , httpd_callback, NULL);

	while (1);

#ifdef WIN32
	WSACleanup();
#endif

	return 0;	
}

int start_httpd(const char* ip, int port, void (*cb)(struct evhttp_request *, void *), void *arg)
{
	//创建event_base和evhttp
	int ret = 0;
	struct event_base* base = event_base_new();
	struct evhttp* httpd = evhttp_new(base);

	tick_st secondtick = {0};
	
	if (!httpd) return 0;
	
	//绑定到指定地址上
	ret = evhttp_bind_socket(httpd, ip, port);
	if (ret != 0) return 0;
	
	//设置事件处理函数
	evhttp_set_gencb(httpd, cb, arg);
	secondtick.tv.tv_sec  = 1;
	secondtick.tv.tv_usec = 0;
	secondtick.timer = evtimer_new(base,ontimer,&secondtick);	
	evtimer_add(secondtick.timer,&secondtick.tv);

	//启动事件循环，当有http请求的时候会调用指定的回调
	event_base_dispatch(base);
	evhttp_free(httpd);

	return 1;
}

void httpd_callback(struct evhttp_request* req, void* arg)
{
	//创建要使用的buffer对象	
	char* uri = NULL;
	char* decoded_uri = NULL; 
	char path[256];
	int  pathlen = 0;
	struct evkeyvalq kvq;		

	//获取请求的URI
	uri = (char*)evhttp_request_get_uri(req);
	evhttp_parse_query(uri,&kvq);

	decoded_uri = evhttp_decode_uri(uri);		
	pathlen = sscanf(decoded_uri,"%[^?]",path);
	free(decoded_uri);
	
	route(router_object(),path,&kvq,req,arg);
}

#define _strftime_ymdhms(timestr){\
	time_t now =time(NULL);\
	strftime(timestr,sizeof(timestr),"%Y-%m-%d %H:%M:%S",localtime(&now));\
}

void mysql_querycallback(void* conn,int code)
{
	time_t now = time(NULL);
	
	if(code == eSqlQueryerr_errorquery||code == eSqlQueryerr_errorping){		
		mysqlquery_t conn = NULL;

		mysqldb_close(mysqlconn);
		conn = mysqldb_connect_reinit(&mysqlconn,g_conf.db_ip,g_conf.db_port,g_conf.db_username,g_conf.db_userpwd,g_conf.db_name);

		if(!mysqldb_isclosed(mysqlconn))
			log_write(g_log, LOG_NOTICE, "connected db ok\n");
		else
			log_write(g_log, LOG_NOTICE, "connected db failed\n");
		
	}else if(code == eSqlConnectOk)
	{
		mysqlquery_t connptr = (mysqlquery_t)conn;
		mysql_query(connptr->m_con, "SET NAMES 'gbk'");

		log_write(g_log, LOG_NOTICE, "connected db ok\n");
	}
}

void ontimer(evutil_socket_t listener,short event,void* arg)
{
	tick_t secondtick = (tick_t)arg; 

	{
		time_t now = time(NULL);

		static int counter = 1;

		if(counter++%60 == 0){	
			if(mysqlconn == NULL){
				char timestr[64];
				strftime(timestr,sizeof(timestr),"%Y-%m-%d %H:%M:%S",localtime(&now));

				mysqlconn = mysqldb_connect_init(g_conf.db_ip,g_conf.db_port,g_conf.db_username,g_conf.db_userpwd,g_conf.db_name,mysql_querycallback);

				if(mysqlconn)
					log_write(g_log, LOG_NOTICE, "connect db ok.\n");
				else
					log_write(g_log, LOG_NOTICE, "connect db failed.\n");
				
			}else{
				int pingcode;
				pingcode = mysqldb_ping(mysqlconn);				
				log_write(g_log, LOG_NOTICE, "ping code %d[%s]\n",pingcode,pingcode==0?"success":"failed");
			}
		}
	}

	evtimer_add(secondtick->timer,&secondtick->tv);
}
