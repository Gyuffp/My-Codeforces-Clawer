#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cf_api_get.h>
#include <stats.h>
#include <html_gen.h>
#include <batch.h>

/* 单用户模式 */
static int single_user_mode() {
    char user_id[256];
    printf("请输入 Codeforces 用户名: ");
    if (scanf("%255s", user_id) != 1) {
        printf("输入错误\n");
        return 1;
    }

    printf("正在抓取 %s 的比赛数据...\n", user_id);

    if (cf_rating_get(user_id) != 0) {
        printf("抓取 rating 数据失败\n");
        return 1;
    }
    printf("[1/3] Rating 数据抓取完成\n");

    if (cf_status_get(user_id) != 0) {
        printf("抓取提交记录失败\n");
        return 1;
    }
    printf("[2/3] 提交记录抓取完成\n");

    User_Stats stats;
    if (compute_user_stats(user_id, &stats) != 0) {
        printf("统计计算失败\n");
        return 1;
    }
    printf("[3/3] 统计计算完成\n");

    char output_path[256];
    snprintf(output_path, sizeof(output_path), "%s_report.html", user_id);
    if (generate_html(&stats, output_path) != 0) {
        printf("HTML 生成失败\n");
        free_user_stats(&stats);
        return 1;
    }

    printf("\n===== 报告摘要 =====\n");
    printf("用户: %s\n", stats.handle);
    printf("Rating: %d (最高 %d)\n", stats.rating, stats.maxRating);
    printf("比赛次数: %d (近180天: %d)\n", stats.contestCount, stats.recent180Count);
    printf("近180天最高 Rating: %d\n", stats.recent180MaxRating);
    printf("总通过题数: %d (近30天: %d, 近180天: %d, 近一年: %d)\n",
           stats.histAll.totalSolved, stats.hist30.totalSolved,
           stats.hist180.totalSolved, stats.histYear.totalSolved);
    printf("\n请在浏览器中打开 %s 查看完整报告\n", output_path);

    free_user_stats(&stats);
    return 0;
}

/* 多用户模式：逐个输入用户名，存入 users.txt，再批量处理 */
static int multi_user_mode() {
    printf("\n多用户模式:\n");
    printf("  1. 从文件读取（如 users.txt）\n");
    printf("  2. 手动输入用户名\n");
    printf("输入选择 (1/2): ");

    int sub;
    if (scanf("%d", &sub) != 1) {
        printf("输入无效\n");
        while (getchar() != '\n');
        return 1;
    }
    getchar();

    if (sub == 1) {
        /* 从文件读取 */
        char path[256];
        printf("请输入文件名: ");
        if (scanf("%255s", path) != 1) {
            printf("输入无效\n"); return 1;
        }
        getchar();

        FILE *test = fopen(path, "r");
        if (!test) { printf("文件 %s 不存在\n", path); return 1; }
        fclose(test);

        printf("正在从 %s 读取用户列表...\n\n", path);
        return batch_process(path);
    }

    if (sub == 2) {
        /* 手动输入 */
        printf("请逐个输入 Codeforces 用户名（输入空行结束）:\n");

        FILE *fp = fopen("users.txt", "w");
        if (!fp) { printf("无法创建 users.txt\n"); return 1; }

        int count = 0;
        char line[256];
        while (1) {
            printf("  用户 %d: ", count + 1);
            if (!fgets(line, sizeof(line), stdin)) break;
            /* 去除首尾空白和换行 */
            char *p = line;
            while (*p == ' ' || *p == '\t') p++;
            char *end = p + strlen(p) - 1;
            while (end >= p && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t'))
                { *end = '\0'; end--; }

            if (strlen(p) == 0) break;

            fprintf(fp, "%s\n", p);
            count++;
        }
        fclose(fp);

        if (count == 0) {
            printf("未输入任何用户名\n");
            remove("users.txt");
            return 0;
        }

        printf("已保存 %d 个用户名到 users.txt\n\n", count);
        return batch_process("users.txt");
    }

    printf("无效选择\n");
    return 1;
}

int main() {
    system("chcp 65001 > nul");  /* 控制台切换到 UTF-8 */

    printf("========================================\n");
    printf("  Codeforces Clawer — 用户比赛分析\n");
    printf("========================================\n");

    while (1) {
        printf("\n请选择模式:\n");
        printf("  1. 单用户模式\n");
        printf("  2. 多用户模式\n");
        printf("  3. 退出\n");
        printf("输入选择 (1/2/3): ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("输入无效，请重新输入\n");
            while (getchar() != '\n'); /* 清空缓冲区 */
            continue;
        }
        getchar(); /* 消耗换行符 */

        switch (choice) {
            case 1:
                single_user_mode();
                break;
            case 2:
                multi_user_mode();
                break;
            case 3:
                printf("再见!\n");
                return 0;
            default:
                printf("无效选择，请输入 1、2 或 3\n");
        }
    }
}
