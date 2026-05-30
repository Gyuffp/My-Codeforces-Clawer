#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cJSON.h>
#include <cf_api_get.h>
#include "stats.h"

/* ==================== 动态字符串数组 ==================== */

typedef struct {
    char **items;
    int count;
    int cap;
} StrVec;

static void sv_init(StrVec *v) {
    v->items = NULL; v->count = 0; v->cap = 0;
}

static void sv_push(StrVec *v, const char *s) {
    if (v->count >= v->cap) {
        v->cap = v->cap ? v->cap * 2 : 8;
        char **tmp = (char **)realloc(v->items, v->cap * sizeof(char *));
        if (!tmp) return;  /* OOM，放弃本次 push */
        v->items = tmp;
    }
    v->items[v->count++] = strdup(s);
}

static int sv_contains(StrVec *v, const char *s) {
    for (int i = 0; i < v->count; i++)
        if (strcmp(v->items[i], s) == 0) return 1;
    return 0;
}

static void sv_free(StrVec *v) {
    for (int i = 0; i < v->count; i++) free(v->items[i]);
    free(v->items);
    v->items = NULL; v->count = 0; v->cap = 0;
}

/* ==================== 综合统计 ==================== */

int compute_user_stats(const char *user_id, User_Stats *stats) {
    memset(stats, 0, sizeof(User_Stats));

    /* --- 1. 获取用户基础信息 --- */
    char *resp = NULL;
    cf_user_get(user_id, &resp);
    if (!resp) { printf("获取用户信息失败\n"); return -1; }

    cJSON *root = cJSON_Parse(resp);
    free(resp);
    if (!root) { printf("用户信息 JSON 解析失败\n"); return -1; }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    if (!result || !cJSON_IsArray(result) || cJSON_GetArraySize(result) == 0) {
        printf("用户信息为空\n");
        cJSON_Delete(root);
        return -1;
    }
    cJSON *user = cJSON_GetArrayItem(result, 0);

    cJSON *jhandle = cJSON_GetObjectItem(user, "handle");
    cJSON *jtitle  = cJSON_GetObjectItem(user, "title");
    cJSON *javatar = cJSON_GetObjectItem(user, "avatar");
    cJSON *jrating = cJSON_GetObjectItem(user, "rating");
    cJSON *jmaxR   = cJSON_GetObjectItem(user, "maxRating");

    stats->handle    = jhandle && cJSON_IsString(jhandle) ? strdup(jhandle->valuestring) : strdup("Unknown");
    stats->title     = jtitle  && cJSON_IsString(jtitle)  ? strdup(jtitle->valuestring)  : strdup("");
    stats->avatar    = javatar && cJSON_IsString(javatar) ? strdup(javatar->valuestring) : strdup("");
    stats->rating    = jrating && cJSON_IsNumber(jrating) ? jrating->valueint : 0;
    stats->maxRating = jmaxR   && cJSON_IsNumber(jmaxR)   ? jmaxR->valueint   : 0;

    if (!stats->title || strlen(stats->title) == 0) {
        free(stats->title);
        stats->title = strdup(rating_to_rank_name(stats->rating));
    }
    cJSON_Delete(root);

    /* --- 2. 解析 rating 历史 --- */
    Contest_Record *c_records = NULL;
    int c_count = 0;
    if (parse_rating_history(user_id, &c_records, &c_count) != 0) {
        printf("解析 rating 历史失败\n");
    }
    stats->totalContests = c_count;
    stats->contestCount = c_count;

    /* --- 3. 解析提交记录 --- */
    Submission_Record *subs = NULL;
    int sub_count = 0;
    if (parse_submissions(user_id, &subs, &sub_count) != 0) {
        printf("解析提交记录失败\n");
    }

    /* --- 4. 计算 180 天统计 --- */
    long long now = (long long)time(NULL);
    long long cutoff = now - 180LL * 24 * 3600;
    int recent180_count = 0;
    int recent180_max = 0;
    for (int i = 0; i < c_count; i++) {
        if (c_records[i].timeSeconds >= cutoff) {
            recent180_count++;
            if (c_records[i].newRating > recent180_max)
                recent180_max = c_records[i].newRating;
        }
    }
    stats->recent180Count = recent180_count;
    stats->recent180MaxRating = recent180_max;

    /* --- 5. 构建每场比赛的解题/补题信息 --- */
    Contest_Summary *summaries = (Contest_Summary *)calloc(c_count, sizeof(Contest_Summary));
    for (int i = 0; i < c_count; i++) {
        summaries[i].contestId   = c_records[i].contestId;
        summaries[i].contestName = strdup(c_records[i].contestName);
        summaries[i].rank        = c_records[i].rank;
        summaries[i].timeSeconds = c_records[i].timeSeconds;
        summaries[i].oldRating   = c_records[i].oldRating;
        summaries[i].newRating   = c_records[i].newRating;
    }

    for (int i = 0; i < c_count; i++) {
        int cid = summaries[i].contestId;
        StrVec solved_contest;  sv_init(&solved_contest);
        StrVec solved_practice; sv_init(&solved_practice);

        /* 第一遍：收集现场通过的题目 */
        for (int j = 0; j < sub_count; j++) {
            if (subs[j].contestId != cid) continue;
            if (strcmp(subs[j].verdict, "OK") != 0) continue;
            if (strlen(subs[j].problemIndex) == 0) continue;
            const char *pt = subs[j].participantType;
            if (strcmp(pt, "CONTESTANT") == 0 || strcmp(pt, "OUT_OF_COMPETITION") == 0) {
                if (!sv_contains(&solved_contest, subs[j].problemIndex))
                    sv_push(&solved_contest, subs[j].problemIndex);
            }
        }
        /* 第二遍：收集赛后补题的题目（排除已现场通过的） */
        for (int j = 0; j < sub_count; j++) {
            if (subs[j].contestId != cid) continue;
            if (strcmp(subs[j].verdict, "OK") != 0) continue;
            if (strlen(subs[j].problemIndex) == 0) continue;
            const char *pt = subs[j].participantType;
            if (strcmp(pt, "PRACTICE") == 0) {
                if (!sv_contains(&solved_contest, subs[j].problemIndex) &&
                    !sv_contains(&solved_practice, subs[j].problemIndex))
                    sv_push(&solved_practice, subs[j].problemIndex);
            }
        }

        summaries[i].solvedInContestCount = solved_contest.count;
        summaries[i].solvedInContest = solved_contest.items;
        solved_contest.items = NULL; solved_contest.count = 0; solved_contest.cap = 0;

        summaries[i].upsolvedCount = solved_practice.count;
        summaries[i].upsolved = solved_practice.items;
        solved_practice.items = NULL; solved_practice.count = 0; solved_practice.cap = 0;

        sv_free(&solved_contest);
        sv_free(&solved_practice);
    }

    stats->contests = summaries;

    /* --- 6. 收集去重题目，记录最新 AC 时间 --- */
    typedef struct { int cid; char idx[8]; int rating; long long latestTime; } Solved;
    Solved *solved_all = NULL;
    int solved_all_cap = 0;
    int solved_all_cnt = 0;

    for (int j = 0; j < sub_count; j++) {
        if (strcmp(subs[j].verdict, "OK") != 0) continue;
        if (subs[j].problemRating <= 0) continue;
        if (strlen(subs[j].problemIndex) == 0) continue;

        /* 查找是否已记录该题 */
        int found = -1;
        for (int k = 0; k < solved_all_cnt; k++) {
            if (solved_all[k].cid == subs[j].contestId &&
                strcmp(solved_all[k].idx, subs[j].problemIndex) == 0) {
                found = k; break;
            }
        }

        if (found >= 0) {
            /* 更新最新 AC 时间 */
            if (subs[j].creationTime > solved_all[found].latestTime)
                solved_all[found].latestTime = subs[j].creationTime;
        } else {
            if (solved_all_cnt >= solved_all_cap) {
                solved_all_cap = solved_all_cap ? solved_all_cap * 2 : 512;
                solved_all = (Solved *)realloc(solved_all, solved_all_cap * sizeof(Solved));
            }
            solved_all[solved_all_cnt].cid = subs[j].contestId;
            strncpy(solved_all[solved_all_cnt].idx, subs[j].problemIndex, 7);
            solved_all[solved_all_cnt].idx[7] = '\0';
            solved_all[solved_all_cnt].rating = subs[j].problemRating;
            solved_all[solved_all_cnt].latestTime = subs[j].creationTime;
            solved_all_cnt++;
        }
    }

    long long cutoffs[] = {
        0,                              /* 全部 */
        now - 365LL * 24 * 3600,        /* 近一年 */
        now - 180LL * 24 * 3600,        /* 近180天 */
        now - 30LL  * 24 * 3600         /* 近30天 */
    };
    Problem_Histogram *hists[] = {
        &stats->histAll, &stats->histYear, &stats->hist180, &stats->hist30
    };

    int bucket_start = 800;
    int bucket_end   = 3600;
    int bucket_step  = 100;
    int nbuckets = (bucket_end - bucket_start) / bucket_step;

    for (int p = 0; p < 4; p++) {
        Rating_Bucket *buckets = (Rating_Bucket *)calloc(nbuckets, sizeof(Rating_Bucket));
        for (int b = 0; b < nbuckets; b++) {
            buckets[b].from  = bucket_start + b * bucket_step;
            buckets[b].to    = buckets[b].from + bucket_step;
            buckets[b].count = 0;
        }

        int total = 0;
        for (int k = 0; k < solved_all_cnt; k++) {
            if (solved_all[k].latestTime < cutoffs[p]) continue;
            total++;
            int r = solved_all[k].rating;
            int b = (r - bucket_start) / bucket_step;
            if (b >= 0 && b < nbuckets) buckets[b].count++;
        }

        hists[p]->buckets = buckets;
        hists[p]->bucketCount = nbuckets;
        hists[p]->totalSolved = total;
    }

    free(solved_all);
    free_contest_records(c_records, c_count);
    free_submissions(subs, sub_count);

    return 0;
}

void free_user_stats(User_Stats *stats) {
    if (!stats) return;
    free(stats->handle);
    free(stats->title);
    free(stats->avatar);
    if (stats->contests) {
        for (int i = 0; i < stats->totalContests; i++) {
            free(stats->contests[i].contestName);
            if (stats->contests[i].solvedInContest) {
                for (int j = 0; j < stats->contests[i].solvedInContestCount; j++)
                    free(stats->contests[i].solvedInContest[j]);
                free(stats->contests[i].solvedInContest);
            }
            if (stats->contests[i].upsolved) {
                for (int j = 0; j < stats->contests[i].upsolvedCount; j++)
                    free(stats->contests[i].upsolved[j]);
                free(stats->contests[i].upsolved);
            }
        }
        free(stats->contests);
    }
    free(stats->histAll.buckets);
    free(stats->histYear.buckets);
    free(stats->hist180.buckets);
    free(stats->hist30.buckets);
    memset(stats, 0, sizeof(User_Stats));
}

/* ==================== 轻量多用户摘要 ==================== */

void free_user_summaries(User_Summary *users, int count) {
    if (!users) return;
    for (int i = 0; i < count; i++) {
        free(users[i].handle);
        free(users[i].title);
    }
    free(users);
}

int compute_summaries(const char *list_path, User_Summary **users_out, int *count_out) {
    *users_out = NULL;
    *count_out = 0;

    /* 读取用户列表 */
    FILE *fp = fopen(list_path, "r");
    if (!fp) { printf("无法打开 %s\n", list_path); return -1; }

    char **names = NULL;
    int names_cap = 0, names_cnt = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\0' || *p == '\n' || *p == '\r') continue;
        char *end = p + strlen(p) - 1;
        while (end >= p && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) {
            *end = '\0'; end--;
        }
        if (strlen(p) == 0) continue;
        if (names_cnt >= names_cap) {
            names_cap = names_cap ? names_cap * 2 : 16;
            char **tmp = (char **)realloc(names, names_cap * sizeof(char *));
            if (!tmp) break;
            names = tmp;
        }
        names[names_cnt++] = strdup(p);
    }
    fclose(fp);

    if (names_cnt == 0) { free(names); return -1; }

    User_Summary *users = (User_Summary *)calloc(names_cnt, sizeof(User_Summary));
    long long now = (long long)time(NULL);
    long long cutoff180 = now - 180LL * 24 * 3600;

    for (int i = 0; i < names_cnt; i++) {
        users[i].handle = strdup(names[i]);
        printf("  [%d/%d] %s\n", i + 1, names_cnt, names[i]);

        /* 获取用户信息 */
        char *resp = NULL;
        cf_user_get(names[i], &resp);
        if (resp) {
            cJSON *root = cJSON_Parse(resp);
            if (root) {
                cJSON *arr = cJSON_GetObjectItem(root, "result");
                if (arr && cJSON_IsArray(arr) && cJSON_GetArraySize(arr) > 0) {
                    cJSON *u = cJSON_GetArrayItem(arr, 0);
                    cJSON *jr = cJSON_GetObjectItem(u, "rating");
                    cJSON *jt = cJSON_GetObjectItem(u, "title");
                    cJSON *jm = cJSON_GetObjectItem(u, "maxRating");
                    users[i].rating    = jr && cJSON_IsNumber(jr) ? jr->valueint : 0;
                    users[i].maxRating = jm && cJSON_IsNumber(jm) ? jm->valueint : 0;
                    if (jt && cJSON_IsString(jt))
                        users[i].title = strdup(jt->valuestring);
                    else
                        users[i].title = strdup(rating_to_rank_name(users[i].rating));
                }
                cJSON_Delete(root);
            }
            free(resp);
        }
        if (!users[i].title || strlen(users[i].title) == 0) {
            free(users[i].title);
            users[i].title = strdup(rating_to_rank_name(users[i].rating));
        }

        /* 拉取 rating 数据，再解析统计 */
        cf_rating_get(names[i]);
        Contest_Record *c_records = NULL;
        int c_count = 0;
        if (parse_rating_history(names[i], &c_records, &c_count) == 0) {
            users[i].contestCount = c_count;
            for (int j = 0; j < c_count; j++) {
                if (c_records[j].timeSeconds >= cutoff180) {
                    users[i].recent180Count++;
                    if (c_records[j].newRating > users[i].recent180MaxRating)
                        users[i].recent180MaxRating = c_records[j].newRating;
                }
            }
            free_contest_records(c_records, c_count);
        }
    }

    for (int i = 0; i < names_cnt; i++) free(names[i]);
    free(names);

    *users_out = users;
    *count_out = names_cnt;
    return 0;
}
