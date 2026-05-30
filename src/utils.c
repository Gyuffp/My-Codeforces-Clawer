#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

/* 根据 rating 返回对应的颜色代码 */
const char *rating_to_color(int rating) {
    if (rating >= 3000) return "#FF0000";
    if (rating >= 2600) return "#FF0000";
    if (rating >= 2400) return "#FF0000";
    if (rating >= 2300) return "#FF8C00";
    if (rating >= 2100) return "#FF8C00";
    if (rating >= 1900) return "#AA00AA";
    if (rating >= 1600) return "#0000FF";
    if (rating >= 1400) return "#03A89E";
    if (rating >= 1200) return "#008000";
    return "#808080";
}

/* 根据 rating 返回对应等级的名称 */
const char *rating_to_rank_name(int rating) {
    if (rating >= 3000) return "Legendary Grandmaster";
    if (rating >= 2600) return "International Grandmaster";
    if (rating >= 2400) return "Grandmaster";
    if (rating >= 2300) return "International Master";
    if (rating >= 2100) return "Master";
    if (rating >= 1900) return "Candidate Master";
    if (rating >= 1600) return "Expert";
    if (rating >= 1400) return "Specialist";
    if (rating >= 1200) return "Pupil";
    return "Newbie";
}

/* 读取文件内容 */
char *read_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { fclose(fp); return NULL; }
    fread(buf, 1, sz, fp);
    buf[sz] = '\0';
    fclose(fp);
    return buf;
}
