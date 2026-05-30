#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cf_api_get.h>
#include <stats.h>
#include <html_gen.h>
#include <batch_html.h>
#include <batch.h>

/* 读取用户列表文件，返回用户名数组 */
static char **read_user_list(const char *path, int *count_out) {
    *count_out = 0;
    FILE *fp = fopen(path, "r");
    if (!fp) { printf("无法打开 %s\n", path); return NULL; }

    char **names = NULL;
    int cap = 0, cnt = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        /* 去除首尾空白和换行 */
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\0' || *p == '\n' || *p == '\r') continue;

        char *end = p + strlen(p) - 1;
        while (end >= p && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t')) {
            *end = '\0'; end--;
        }

        if (strlen(p) == 0) continue;

        if (cnt >= cap) {
            cap = cap ? cap * 2 : 16;
            char **tmp = (char **)realloc(names, cap * sizeof(char *));
            if (!tmp) break;
            names = tmp;
        }
        names[cnt++] = strdup(p);
    }
    fclose(fp);

    *count_out = cnt;
    return names;
}

/* 批量处理入口：读取用户列表 → 阶段1生成index.html → 阶段2生成个人报告 */
int batch_process(const char *list_path) {
    /* --- 1. 读取用户列表 --- */
    int user_count = 0;
    char **users = read_user_list(list_path, &user_count);
    if (!users || user_count == 0) {
        printf("用户列表为空或无法读取\n");
        return -1;
    }
    printf("共读取 %d 个用户名\n", user_count);

    /* --- 2. 轻量统计（不拉 status）--- */
    printf("\n[第一阶段] 获取用户基础信息...\n");
    User_Summary *summaries = NULL;
    int summary_count = 0;
    if (compute_summaries(list_path, &summaries, &summary_count) != 0) {
        printf("统计摘要计算失败\n");
        for (int i = 0; i < user_count; i++) free(users[i]);
        free(users);
        return -1;
    }

    /* --- 3. 生成 index.html --- */
    generate_index_html(summaries, summary_count, "index.html");

    /* --- 4. 逐个生成详细报告 --- */
    printf("\n[第二阶段] 生成个人详细报告...\n");
    for (int i = 0; i < user_count; i++) {
        printf("[%d/%d] 正在生成 %s 的报告...\n", i + 1, user_count, users[i]);

        /* 拉取 status（如果还没有） */
        cf_status_get(users[i]);

        /* 计算完整统计 */
        User_Stats stats;
        if (compute_user_stats(users[i], &stats) != 0) {
            printf("  警告: %s 统计计算失败，跳过\n", users[i]);
            continue;
        }

        /* 生成 HTML */
        char path[256];
        snprintf(path, sizeof(path), "%s_report.html", users[i]);
        generate_html(&stats, path);
        free_user_stats(&stats);
    }

    /* --- 5. 清理 --- */
    free_user_summaries(summaries, summary_count);
    for (int i = 0; i < user_count; i++) free(users[i]);
    free(users);

    printf("\n===== 批量处理完成 =====\n");
    printf("请在浏览器中打开 index.html 查看总览\n");
    return 0;
}
