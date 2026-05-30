#ifndef STATS_H
#define STATS_H

#include "parse.h"

/* 综合所有数据计算 User_Stats（含 status，较重） */
int compute_user_stats(const char *user_id, User_Stats *stats);
void free_user_stats(User_Stats *stats);

/* 轻量统计：仅用 user.info + user.rating，生成多用户总览表行 */
int compute_summaries(const char *list_path, User_Summary **users_out, int *count_out);

#endif
