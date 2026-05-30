#ifndef BATCH_H
#define BATCH_H

/* 批量处理入口：读取 users.txt，拉取数据，生成 index.html + 各用户报告 */
int batch_process(const char *list_path);

#endif
