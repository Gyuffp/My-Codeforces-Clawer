#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batch_html.h"

/* 生成多用户总览表 index.html */
int generate_index_html(User_Summary *users, int count, const char *output_path) {
    FILE *fp = fopen(output_path, "w");
    if (!fp) { printf("无法创建 %s\n", output_path); return -1; }

    fprintf(fp, "\xEF\xBB\xBF<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n"
          "<meta charset=\"UTF-8\">\n"
          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
          "<title>Codeforces 用户总览</title>\n"
          "<style>\n"
          "*{margin:0;padding:0;box-sizing:border-box}\n"
          "body{font-family:'Segoe UI',sans-serif;background:#1a1a2e;color:#eee;min-height:100vh}\n"
          ".header{background:#16213e;padding:24px 32px;border-bottom:3px solid #0f3460}\n"
          ".header h1{font-size:1.6em}\n"
          ".header p{color:#888;margin-top:4px;font-size:0.9em}\n"
          ".container{padding:24px;max-width:1400px;margin:0 auto}\n"
          "table{width:100%%;border-collapse:collapse;font-size:0.95em}\n"
          "thead{background:#0f3460}\n"
          "th{padding:14px 12px;text-align:left;font-weight:600;white-space:nowrap}\n"
          "td{padding:12px;border-bottom:1px solid #1a1a3e;white-space:nowrap}\n"
          "tr:hover{background:rgba(233,69,96,0.08)}\n"
          "a{text-decoration:none;font-weight:bold}\n"
          "a:hover{text-decoration:underline}\n"
          ".badge-title{display:inline-block;padding:2px 8px;border-radius:4px;font-size:0.8em;margin-left:8px;opacity:0.8}\n"
          ".num{font-variant-numeric:tabular-nums}\n"
          ".footer{text-align:center;color:#666;padding:20px;font-size:0.8em}\n"
          "@media(max-width:768px){.container{padding:8px}table{font-size:0.75em}th,td{padding:6px 4px}}\n"
          "</style>\n</head>\n<body>\n"
          "<div class=\"header\"><h1>Codeforces 用户总览</h1>"
          "<p>共 %d 位用户 | 点击用户名查看详细报告</p></div>\n"
          "<div class=\"container\">\n<table>\n<thead><tr>"
          "<th>#</th><th>用户名</th><th>头衔</th><th>当前 Rating</th>"
          "<th>最高 Rating</th><th>比赛次数</th>"
          "<th>近180天次数</th><th>近180天最高</th></tr></thead>\n<tbody>\n",
          count);

    for (int i = 0; i < count; i++) {
        User_Summary *u = &users[i];
        const char *color = rating_to_color(u->rating);
        char report[256];
        snprintf(report, sizeof(report), "%s_report.html", u->handle);

        fprintf(fp, "<tr>");
        fprintf(fp, "<td class=\"num\">%d</td>", i + 1);
        fprintf(fp, "<td><a href=\"%s\" style=\"color:%s\">%s</a></td>",
                report, color, u->handle);
        fprintf(fp, "<td><span class=\"badge-title\" style=\"background:%s;color:#fff\">%s</span></td>",
                color, u->title);
        fprintf(fp, "<td><span style=\"color:%s;font-weight:bold\">%d</span></td>",
                color, u->rating);
        fprintf(fp, "<td style=\"color:%s\">%d</td>",
                rating_to_color(u->maxRating), u->maxRating);
        fprintf(fp, "<td class=\"num\">%d</td>", u->contestCount);
        fprintf(fp, "<td class=\"num\">%d</td>", u->recent180Count);
        fprintf(fp, "<td style=\"color:%s\">%d</td>",
                rating_to_color(u->recent180MaxRating), u->recent180MaxRating);
        fprintf(fp, "</tr>\n");
    }

    fprintf(fp, "</tbody></table>\n<div class=\"footer\">"
            "数据来源: <a href=\"https://codeforces.com/\" style=\"color:#e94560\">Codeforces API</a></div>\n"
            "</div>\n</body>\n</html>");

    fclose(fp);
    printf("多用户总览表已生成: %s\n", output_path);
    return 0;
}
