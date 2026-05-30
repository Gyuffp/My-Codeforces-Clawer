#ifndef CF_API_GET_H
#define CF_API_GET_H

/* 获取用户信息，返回JSON字符串（调用者需释放 *response） */
void cf_user_get(const char *user_id, char **response);

/* 获取用户rating历史，保存到 <user_id>_rating.json，返回0成功 */
int cf_rating_get(const char *user_id);

/* 获取用户提交记录，保存到 <user_id>_status.json，返回0成功 */
int cf_status_get(const char *user_id);

#endif
