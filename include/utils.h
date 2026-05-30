#ifndef UTILS_H
#define UTILS_H

/* 根据 rating 返回 CF 颜色 */
const char *rating_to_color(int rating);

/* 根据 rating 返回 CF 头衔 */
const char *rating_to_rank_name(int rating);

/* 读取文件全部内容到字符串（调用者需 free） */
char *read_file(const char *path);

#endif
