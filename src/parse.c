#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>
#include "parse.h"

/* 比较函数：按时间降序 */
static int cmp_contest_time_desc(const void *a, const void *b) {
    const Contest_Record *ca = (const Contest_Record *)a;
    const Contest_Record *cb = (const Contest_Record *)b;
    if (ca->timeSeconds < cb->timeSeconds) return 1;
    if (ca->timeSeconds > cb->timeSeconds) return -1;
    return 0;
}

/* ==================== 解析 rating 历史 ==================== */

int parse_rating_history(const char *user_id, Contest_Record **contests_out, int *count_out) {
    *contests_out = NULL;
    *count_out = 0;

    char path[256];
    snprintf(path, sizeof(path), "%s_rating.json", user_id);
    char *json = read_file(path);
    if (!json) { printf("无法读取 rating 文件: %s\n", path); return -1; }

    cJSON *root = cJSON_Parse(json);
    free(json);
    if (!root) { printf("rating JSON 解析失败\n"); return -1; }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    if (!result || !cJSON_IsArray(result)) {
        printf("rating JSON 中无 result 数组\n");
        cJSON_Delete(root);
        return -1;
    }

    int count = cJSON_GetArraySize(result);
    Contest_Record *contests = (Contest_Record *)calloc(count, sizeof(Contest_Record));
    if (!contests) { cJSON_Delete(root); return -1; }

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);
        cJSON *cid   = cJSON_GetObjectItem(item, "contestId");
        cJSON *cname = cJSON_GetObjectItem(item, "contestName");
        cJSON *rank  = cJSON_GetObjectItem(item, "rank");
        cJSON *ts    = cJSON_GetObjectItem(item, "ratingUpdateTimeSeconds");
        cJSON *oldR  = cJSON_GetObjectItem(item, "oldRating");
        cJSON *newR  = cJSON_GetObjectItem(item, "newRating");

        contests[i].contestId   = cid  && cJSON_IsNumber(cid)  ? cid->valueint  : 0;
        contests[i].rank        = rank && cJSON_IsNumber(rank) ? rank->valueint : 0;
        contests[i].timeSeconds = ts   && cJSON_IsNumber(ts)   ? (long long)ts->valuedouble : 0;
        contests[i].oldRating   = oldR && cJSON_IsNumber(oldR) ? oldR->valueint : 0;
        contests[i].newRating   = newR && cJSON_IsNumber(newR) ? newR->valueint : 0;
        if (cname && cJSON_IsString(cname))
            contests[i].contestName = strdup(cname->valuestring);
        else
            contests[i].contestName = strdup("Unknown");
    }

    cJSON_Delete(root);
    qsort(contests, count, sizeof(Contest_Record), cmp_contest_time_desc);

    *contests_out = contests;
    *count_out = count;
    return 0;
}

void free_contest_records(Contest_Record *contests, int count) {
    if (!contests) return;
    for (int i = 0; i < count; i++) free(contests[i].contestName);
    free(contests);
}

/* ==================== 解析 submissions ==================== */

int parse_submissions(const char *user_id, Submission_Record **subs_out, int *count_out) {
    *subs_out = NULL;
    *count_out = 0;

    char path[256];
    snprintf(path, sizeof(path), "%s_status.json", user_id);
    char *json = read_file(path);
    if (!json) { printf("无法读取 status 文件: %s\n", path); return -1; }

    cJSON *root = cJSON_Parse(json);
    free(json);
    if (!root) { printf("status JSON 解析失败\n"); return -1; }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    if (!result || !cJSON_IsArray(result)) {
        printf("status JSON 中无 result 数组\n");
        cJSON_Delete(root);
        return -1;
    }

    int count = cJSON_GetArraySize(result);
    Submission_Record *subs = (Submission_Record *)calloc(count, sizeof(Submission_Record));
    if (!subs) { cJSON_Delete(root); return -1; }

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(result, i);

        cJSON *cid = cJSON_GetObjectItem(item, "contestId");
        cJSON *ct  = cJSON_GetObjectItem(item, "creationTimeSeconds");
        cJSON *vd  = cJSON_GetObjectItem(item, "verdict");

        subs[i].contestId    = cid && cJSON_IsNumber(cid) ? cid->valueint : 0;
        subs[i].creationTime = ct  && cJSON_IsNumber(ct)  ? (long long)ct->valuedouble : 0;

        if (vd && cJSON_IsString(vd)) subs[i].verdict = strdup(vd->valuestring);
        else subs[i].verdict = strdup("");

        cJSON *prob = cJSON_GetObjectItem(item, "problem");
        if (prob && cJSON_IsObject(prob)) {
            cJSON *pIdx = cJSON_GetObjectItem(prob, "index");
            cJSON *pName = cJSON_GetObjectItem(prob, "name");
            cJSON *pRat  = cJSON_GetObjectItem(prob, "rating");
            if (pIdx && cJSON_IsString(pIdx)) subs[i].problemIndex = strdup(pIdx->valuestring);
            else subs[i].problemIndex = strdup("");
            if (pName && cJSON_IsString(pName)) subs[i].problemName = strdup(pName->valuestring);
            else subs[i].problemName = strdup("");
            subs[i].problemRating = (pRat && cJSON_IsNumber(pRat)) ? pRat->valueint : 0;
        } else {
            subs[i].problemIndex = strdup("");
            subs[i].problemName = strdup("");
            subs[i].problemRating = 0;
        }

        cJSON *auth = cJSON_GetObjectItem(item, "author");
        if (auth && cJSON_IsObject(auth)) {
            cJSON *pt = cJSON_GetObjectItem(auth, "participantType");
            if (pt && cJSON_IsString(pt)) subs[i].participantType = strdup(pt->valuestring);
            else subs[i].participantType = strdup("");
        } else {
            subs[i].participantType = strdup("");
        }
    }

    cJSON_Delete(root);
    *subs_out = subs;
    *count_out = count;
    return 0;
}

void free_submissions(Submission_Record *subs, int count) {
    if (!subs) return;
    for (int i = 0; i < count; i++) {
        free(subs[i].problemIndex);
        free(subs[i].problemName);
        free(subs[i].verdict);
        free(subs[i].participantType);
    }
    free(subs);
}
