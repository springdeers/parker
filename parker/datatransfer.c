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

// ��tbHistoricExperience����ת���������
static int tbHistoricExperience_transfer(char * cardid, scores_st scores)
{
	char *ordertime, *scoretime, *username, *phoneno, *openid;

	// 1. ���㲢���ɳɼ�json
	scores_shrink(&scores);			// ��scores�ṹ����о���ͬһ������ֻ��Ψһ�ĳɼ���ȡ���ֵ��

	// ��ȡ���г���������ѧ���ܺ�
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

	// ��ȡδ���鳡���������json��
	// json����ʽ�� {"remains":[{"name":"��������"},{"name":"������ˮ"},{"name":"����С��"}]}
	if (-1 == db_query_remains(sqlobj_venue_db, &scores))
	{
		printf("db_query_credits failed.\n");
		return -1;
	}

	// ��ȡ�ο�����
	if (-1 == db_query_travelername(sqlobj_userinfo_db, atoi(cardid), &username))
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

	// 2. ��ȡ�����ɹ켣json
	positions_st positions = { 0 };
	db_load_positions(sqlobj_userinfo_db, atoi(cardid), &positions);
	// to be continued ...
	// ����positions�����ɹ켣json��



	
	// 3. ��ȡ����Ҫ��


	// 4. ����venue_db���tbHistoricExperience����



}

// ��user_info_db����ʱ�洢��tbVisitorScore��������ת�Ƶ�venue_db���е�tbVisitorScore����
static int tbVisitorScore_transfer(char * cardid, scores_t scores)
{
	if (NULL == scores)
	{
		return -1;
	}
}

// ��user_info_db����ʱ�洢��tbVisitorActivity��������ת�Ƶ�venue_db���е�tbVisitorActivity����
static int tb_VisitorActivity_transfer(char * cardidi, visitrslt_t activities)
{
	if (NULL == activities)
	{
		return -1;
	}
}

void dojob(job_t job)
{
	if (job == NULL) return;

	char *cardid = job->cardid;
	scores_st scores;
	visitrslt_st activities[128];

	// �����scene_server���ʱͬʱ�������ݵ���ͬ�Ŀ��У���1��2��������������ڴ˴���ȥ

	// 1. ��user_info_db���е�tbvisitoractivity���������ת�Ƶ�venue_db���е�tbvisitoractivity����
	db_load_scores(sqlobj_venue_db, atoi(cardid), &scores);		
	tbVisitorScore_transfer(cardid, &scores);

	// 2. ��user_info_db���е�tbvisitorscore���������ת�Ƶ�venue_db���е�tbvisitorscore����
	db_load_visitor_activity(sqlobj_venue_db, atoi(cardid), activities, sizeof(activities) / sizeof(activities[0]));
	tb_VisitorActivity_transfer(cardid, activities);

	// 3. ��venue_db�⣺tbHistoricExperience���в�������
	tbHistoricExperience_transfer(cardid,scores);
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
			abstime_set(abstime, 100);//δ��1000msʱ�̡�
			
			wait = pthread_cond_timedwait(&transfer->_cond, &transfer->_mt_signal, &abstime);//ע��,�ú����ȴ���ʱ�䲻����ȷ��
		}

		if (wait != 0 && wait != ETIMEDOUT) {//error
			pthread_mutex_unlock(&transfer->_mt_signal);
			break; 
		}

		while (jqueue_size(transfer->_workqueue) > 0){
			jqueue_push(transfer->_workingqueue, jqueue_pull(transfer->_workqueue), 0);
		}
		
		pthread_mutex_unlock(&transfer->_mt_signal);
		
		//����������,��ֹ�������߳������������߳�.
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
