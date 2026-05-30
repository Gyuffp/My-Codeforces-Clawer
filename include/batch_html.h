#ifndef BATCH_HTML_H
#define BATCH_HTML_H

#include "parse.h"

/* 生成多用户总览表 index.html */
int generate_index_html(User_Summary *users, int count, const char *output_path);

#endif
