#include "datatransfer.h"
#include "dbaccess.h"
#include "event2/buffer.h"
#include "membuff.h"
#include "posrslt.h"
#include <sys/timeb.h>

extern mysqlquery_t sqlobj_venue_db;
extern mysqlquery_t sqlobj_userinfo_db;
extern log_t g_log;
static membuff_t g_membuffer_score = NULL;

#define abstime_set(__time_,__timeout_ms_)\
{\
	struct _timeb currSysTime;\
	_ftime(&currSysTime);\
	__time_.tv_sec = currSysTime.time + (currSysTime.millitm + __timeout_ms_) / 1000; \
	__time_.tv_nsec = ((currSysTime.millitm + __timeout_ms_) % 1000) * 1000000;\
}

static void dojob(job_t job);

// 向tbHistoricExperience表中转移相关数据
static int tbHistoricExperience_transfer(int cardid, scores_st scores)
{
	char *ordertime, *scoretime, *username, *phoneno, *openid;

	// 1. 计算并生成成绩json
	scores_shrink(&scores);			// 对scores结构体进行精简，同一个场景只出唯一的成绩（取最大值）

	// 获取所有场景个数与学分总和
	if (-1 == db_query_credits(sqlobj_venue_db, &scores.totalscenes, &scores.totalcredit))
	{
		printf("db_query_credits failed.\n");
		return -1;
	}

	if (scores.totalcredit == 0)
	{		
		printf("error: scores.totalcredit == 0.\n");
		return -1;
	}

	// 获取未体验场景，构造出json串
	// json串格式： {"remains":[{"name":"烟雾逃生"},{"name":"汽车落水"},{"name":"地震小屋"}]}
	if (-1 == db_query_remains(sqlobj_venue_db, &scores))
	{
		printf("db_query_credits failed.\n");
		return -1;
	}

	// 获取游客姓名
	if (-1 == db_query_travelername(sqlobj_userinfo_db,cardid, &username))
	{
		printf("db_query_travelrname failed.\n");
		return -1;
	}

	finalscore_st finalscore = { 0 };
	memcpy(finalscore.name, username, strlen(username) + 1);
	free(username);

	get_finalscore(&scores, &finalscore);

	_assure_clearbuff(g_membuffer_score, 4096);

	membuff_add_printf(g_membuffer_score, "{\"name\":\"%s\",\"type\":\"score\",\"code\":\"0\",\"cardid\":\"%s\",\"comid\":\"%s\",\"finalscore\":\"%.1f\",\"credit_point\":\"%.1f\",\"grade_point\":\"%.1f\",\"class\":\"%s\",\"suggestion\":\"%s\",\"remainsobj\":%s}",
		finalscore.name,
		cardid,
		"",
		finalscore.finalscore,
		finalscore.credit_point,
		finalscore.grade_point,
		finalscore.scoreclass,
		finalscore.suggestion,
		finalscore.rmnscenes
		);

	membuff_addchar(g_membuffer_score, '\0');

	// 2. 获取并生成轨迹json
	positions_st positions = { 0 };
	db_load_positions(sqlobj_userinfo_db, cardid, &positions);
	// to be continued ...
	// 根据positions，生成轨迹json串



	
	// 3. 获取其他要素


	// 4. 存入venue_db库的tbHistoricExperience表中



}

static int tbAnalyse_transfer(int cardid)
{

}


static int stage_one_dojob(int cardid)
{
	// 1. 删除user_info_db.tbtagmapping表中cardid对应记录
	db_clear_tagmapping(sqlobj_userinfo_db, cardid);

	// 2. 删除user_info_db.tbtraveler表中cardid对应记录（会触发tbtravlergroup表的变动？）
	db_clear_traveler(sqlobj_userinfo_db, cardid);

	// 3. 删除user_info_db.tbtagpos表中cardid对应记录
	db_clear_visitor_tagpos(sqlobj_userinfo_db, cardid);

	// 4. 删除venue_db.visitoractivity表中cardid对应记录
	db_clear_visitor_activtiy(sqlobj_venue_db, cardid);

	// 5. 删除venue_db.visitorscore表中cardid对应记录
	db_clear_scores(sqlobj_venue_db, cardid);
}


static int stage_two_dojob(int cardid)
{
	/*********** 第一步，相关数据提取整合与转移 ************/
	scores_st scores;
	db_load_scores(sqlobj_venue_db, cardid, &scores);

	// 1. 向venue_db.tbAnalyse表中插入数据，供统计分析
	// 数据源自user_info_db.tbTravelrScore表
	tbAnalyse_transfer(cardid, scores);

	// 2. 向venue_db.tbHistoricExperience表中插入数据，供历史查询
	// 数据源自user_info_db.tbTravelrScore、tbtagpos、tbtagmapping、tbsettledorder、tbtraveler表等
	tbHistoricExperience_transfer(cardid, scores);

	/*********** 第二步，删除卡号相关各种数据，实现退卡 ************/

	// 3. 删除user_info_db.tbtagmapping表中cardid对应记录
	db_clear_tagmapping(sqlobj_userinfo_db, cardid);

	// 4. 删除user_info_db.tbtraveler表中cardid对应记录（会触发tbtravlergroup表的变动？）
	db_clear_traveler(sqlobj_userinfo_db, cardid);

	// 5. 删除user_info_db.tbtagpos表中cardid对应记录
	db_clear_visitor_tagpos(sqlobj_userinfo_db, cardid);

	// 6. 删除user_info_db.tbTravelerScore表中cardid对应记录
	db_clear_travelerscores(sqlobj_userinfo_db, cardid);
}

#define STAGE_ONE	1	

void dojob(job_t job)
{
	if (job == NULL) return;

	int cardid = atoi(job->cardid);

#if STAGE_ONE

	// 阶段一的实现：仅仅实现前台自动退卡系统的退卡与成绩打印。原系统无法查询历史成绩（与轨迹）。
	// 所以此处仅实现退卡与原数据的删除即可，无需转移任何数据

	stage_one_dojob(cardid);

#else

	// 阶段二的实现：最终升级后需要做的工作，即采用新的数据库，新的流程实现数据提取、整合、转移与退卡操作
	// 该阶段下，venue_db.visitoractivity弃用，visitorscore表由venue_db库移动到user_info_db中

	stage_two_dojob(cardid);	

#endif
}

static void* _worker(void* param)
{
	transfer_t transfer = (transfer_t)param;

	if (transfer == NULL) return NULL;

	struct timespec abstime = {0};
	
	while (transfer->_runing){
		pthread_mutex_lock(&transfer->_mt_signal);

		int wait = 0;
		
		while (jqueue_size(transfer->_workqueue) == 0 && wait == 0 )//predicate in critical area..
		{
			abstime_set(abstime, 300);//未来300ms时刻。
			
			wait = pthread_cond_timedwait(&transfer->_cond, &transfer->_mt_signal, &abstime);
		}

		if (wait != 0 && wait != ETIMEDOUT) {//error
			pthread_mutex_unlock(&transfer->_mt_signal);
			break; 
		}

		while (jqueue_size(transfer->_workqueue) > 0){
			jqueue_push(transfer->_workingqueue, jqueue_pull(transfer->_workqueue), 0);
		}
		
		pthread_mutex_unlock(&transfer->_mt_signal);
		
		//非锁定区域,防止消费者线程阻塞生产者线程.
		printf("task inqueue is %d.\n", jqueue_size(transfer->_workingqueue));
		while (jqueue_size(transfer->_workingqueue) > 0){
			job_t job = (job_t)jqueue_pull(transfer->_workingqueue);
			dojob(job);
			free(job);
		}
	}

	transfer_clear(transfer);

	return (void*)1;
}

transfer_t transfer_init(transfer_t transfer)
{
	if (transfer == NULL) return NULL;

	transfer->_ctime = time(0);

	transfer->_workqueue = jqueue_new();
	transfer->_workingqueue = jqueue_new();

	pthread_cond_init(&transfer->_cond, NULL);

	pthread_mutex_init(&transfer->_mt_signal, NULL);

	return transfer;
}

transfer_t transfer_clear(transfer_t transfer)
{
	if (transfer == NULL) return NULL;

	if (transfer->_workqueue)    jqueue_free(transfer->_workqueue);
	if (transfer->_workingqueue) jqueue_free(transfer->_workingqueue);

	transfer->_pth = NULL;
	transfer->_runing = 0;

	pthread_mutex_destroy(&transfer->_mt_signal);

	pthread_cond_destroy(&transfer->_cond);

	return transfer;
}

transfer_t transfer_new()
{
	transfer_t rt = NULL;

	while ((rt = calloc(1, sizeof(transfer_st))) == NULL) Sleep(1);

	transfer_init(rt);
	
	return rt;
}

void       transfer_free(transfer_t transfer)
{
	if (transfer)
	{
		transfer_stop(transfer);

		transfer_clear(transfer);
	}
}

int        transfer_start(transfer_t transfer)
{
	if (transfer == NULL)       return 0;
	if (transfer->_pth != NULL) return 0;

	transfer->_runing = 1;

	pthread_create(&transfer->_pth, NULL, _worker, transfer);

	return 1;
}

int        transfer_stop(transfer_t transfer)
{
	if (transfer == NULL || transfer->_pth == NULL)  return 0;

	transfer->_runing = 0;

	pthread_join(transfer->_pth, NULL);

	transfer->_pth = NULL;

	return 1;
}

int      transfer_postAjob(transfer_t transfer, job_st job)
{
	if (transfer == NULL) return 0;

	pthread_mutex_lock(&transfer->_mt_signal);

	job_t job2 = calloc(1, sizeof(job_st));
	memcpy(job2, &job, sizeof(job_st));

	jqueue_push(transfer->_workqueue, job2,0);

	pthread_cond_signal(&transfer->_cond);

	pthread_mutex_unlock(&transfer->_mt_signal);

	return 1;
}

int  transfer_jobs_remain_cnt(transfer_t transfer)
{
	if (transfer == 0) return 0;

	return jqueue_size(transfer->_workingqueue) + jqueue_size(transfer->_workqueue);
}
