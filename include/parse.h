#ifndef PARSE_H
#define PARSE_H

#include "utils.h"

/* ---------- 数据结构 ---------- */

/* 单场比赛记录（来自 user.rating） */
typedef struct {
    int contestId;
    char *contestName;
    int rank;
    long long timeSeconds;      /* ratingUpdateTimeSeconds */
    int oldRating;
    int newRating;
} Contest_Record;

/* 单条提交记录（来自 user.status，只存关键字段） */
typedef struct {
    int contestId;
    char *problemIndex;         /* "A", "B", "C1"... */
    char *problemName;
    int problemRating;          /* 题目难度等级分，0 表示无 */
    char *verdict;              /* "OK" 等 */
    char *participantType;      /* "CONTESTANT" / "PRACTICE" / "VIRTUAL" / "OUT_OF_COMPETITION" */
    long long creationTime;     /* 提交时间戳 */
} Submission_Record;

/* 单场比赛汇总：rating变化 + 现场/补题情况 */
typedef struct {
    int contestId;
    char *contestName;
    int rank;
    long long timeSeconds;
    int oldRating;
    int newRating;
    /* 现场通过的题目索引列表 */
    char **solvedInContest;
    int solvedInContestCount;
    /* 赛后补题通过的题目索引列表 */
    char **upsolved;
    int upsolvedCount;
} Contest_Summary;

/* 按难度等级分的直方图区间 */
typedef struct {
    int from;       /* 区间下限（含） */
    int to;         /* 区间上限（不含） */
    int count;      /* 该区间通过题数 */
} Rating_Bucket;

/* 多用户总览表行数据（不拉 status，仅 user.info + user.rating） */
typedef struct {
    char *handle;
    int rating;
    char *title;
    int maxRating;
    int contestCount;
    int recent180Count;
    int recent180MaxRating;
} User_Summary;

void free_user_summaries(User_Summary *users, int count);

/* 一个时间段的直方图 */
typedef struct {
    int totalSolved;
    Rating_Bucket *buckets;
    int bucketCount;
} Problem_Histogram;

/* 用户完整统计信息 */
typedef struct {
    char *handle;
    char *title;
    char *avatar;
    int rating;
    int maxRating;
    int contestCount;
    int recent180Count;         /* 近180天比赛次数 */
    int recent180MaxRating;     /* 近180天最高等级分 */
    Problem_Histogram histAll;  /* 全部通过题目 */
    Problem_Histogram histYear; /* 最近一年 */
    Problem_Histogram hist180;  /* 最近180天 */
    Problem_Histogram hist30;   /* 最近30天 */
    Contest_Summary *contests;  /* 所有比赛（时间倒序） */
    int totalContests;
} User_Stats;

/* ---------- 解析函数 ---------- */

int parse_rating_history(const char *user_id, Contest_Record **contests, int *count);
void free_contest_records(Contest_Record *contests, int count);

int parse_submissions(const char *user_id, Submission_Record **subs, int *count);
void free_submissions(Submission_Record *subs, int count);

#endif
