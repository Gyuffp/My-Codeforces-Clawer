#ifndef HTML_GEN_H
#define HTML_GEN_H

#include "parse.h"

/* 生成 HTML 可视化报告 */
int generate_html(const User_Stats *stats, const char *output_path);

#endif
