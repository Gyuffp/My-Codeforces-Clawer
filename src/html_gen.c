#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "html_gen.h"

/* 时间戳转日期字符串 "YYYY-MM-DD" */
static const char *time_to_date_str(long long ts) {
    static char buf[32];
    time_t t = (time_t)ts;
    struct tm *tm_info = localtime(&t);
    if (!tm_info) { snprintf(buf, sizeof(buf), "N/A"); return buf; }
    strftime(buf, sizeof(buf), "%Y-%m-%d", tm_info);
    return buf;
}

int generate_html(const User_Stats *stats, const char *output_path) {
    FILE *fp = fopen(output_path, "w");
    if (!fp) { printf("无法创建输出文件: %s\n", output_path); return -1; }

    fputs("\xEF\xBB\xBF<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n"
          "<meta charset=\"UTF-8\">\n"
          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
          "<title>Codeforces 用户分析</title>\n"
          "<script src=\"https://cdn.jsdelivr.net/npm/echarts@5.5.0/dist/echarts.min.js\"></script>\n"
          "<style>\n"
          "*{margin:0;padding:0;box-sizing:border-box}\n"
          "body{font-family:'Segoe UI',sans-serif;background:#1a1a2e;color:#eee;min-height:100vh}\n"
          ".header{background:#16213e;padding:30px;display:flex;align-items:center;gap:24px;border-bottom:3px solid #0f3460}\n"
          ".avatar{width:100px;height:100px;border-radius:50%;border:3px solid #e94560}\n"
          ".user-info h1{font-size:2em;margin-bottom:4px}\n"
          ".user-info .title{font-size:1.1em;color:#aaa;margin-bottom:8px}\n"
          ".stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px;padding:24px}\n"
          ".stat-card{background:#16213e;border-radius:12px;padding:20px;text-align:center;border-left:4px solid}\n"
          ".stat-value{font-size:2em;font-weight:bold}\n"
          ".stat-label{font-size:0.85em;color:#888;margin-top:4px}\n"
          ".section{padding:24px}\n"
          ".section h2{font-size:1.4em;margin-bottom:16px;padding-bottom:8px;border-bottom:2px solid #0f3460}\n"
          ".chart-container{background:#16213e;border-radius:12px;padding:16px;margin-bottom:24px}\n"
          ".chart{width:100%;height:450px}\n"
          "table{width:100%;border-collapse:collapse;font-size:0.9em}\n"
          "thead{background:#0f3460}\n"
          "th{padding:12px 10px;text-align:left;font-weight:600;white-space:nowrap}\n"
          "td{padding:10px;border-bottom:1px solid #1a1a3e;white-space:nowrap}\n"
          "tr:hover{background:rgba(233,69,96,0.1)}\n"
          ".badge{display:inline-block;padding:2px 8px;border-radius:4px;font-size:0.8em;margin:1px;font-weight:600}\n"
          ".badge-ok{background:#2ecc71;color:#fff}\n"
          ".badge-up{background:#e67e22;color:#fff}\n"
          ".rate-up{color:#2ecc71}\n"
          ".rate-down{color:#e74c3c}\n"
          ".hist-grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}\n"
          ".hist-chart{width:100%;height:350px}\n"
          "@media(max-width:768px){.header{flex-direction:column;text-align:center}"
          ".hist-grid{grid-template-columns:1fr}}\n"
          "</style>\n</head>\n<body>\n", fp);

    /* ===== Header ===== */
    const char *color = rating_to_color(stats->rating);
    fprintf(fp, "<div class=\"header\">\n");
    fprintf(fp, "<img class=\"avatar\" src=\"%s\" alt=\"avatar\" onerror=\"this.style.display='none'\">\n",
            stats->avatar ? stats->avatar : "");
    fprintf(fp, "<div class=\"user-info\">\n");
    if (stats->rating >= 3000 && stats->handle && strlen(stats->handle) > 0) {
        fprintf(fp, "<h1><span style=\"color:#000\">%c</span><span style=\"color:%s\">%s</span></h1>\n",
                stats->handle[0], color, stats->handle + 1);
    } else {
        fprintf(fp, "<h1 style=\"color:%s\">%s</h1>\n", color, stats->handle);
    }
    fprintf(fp, "<div class=\"title\">%s</div>\n", stats->title);
    fprintf(fp, "</div></div>\n");

    /* ===== 统计卡片 ===== */
    fprintf(fp, "<div class=\"stats-grid\">\n");

    fprintf(fp, "<div class=\"stat-card\" style=\"border-color:%s\">", color);
    fprintf(fp, "<div class=\"stat-value\" style=\"color:%s\">%d</div>", color, stats->rating);
    fprintf(fp, "<div class=\"stat-label\">当前 Rating</div></div>\n");

    fprintf(fp, "<div class=\"stat-card\" style=\"border-color:%s\">", rating_to_color(stats->maxRating));
    fprintf(fp, "<div class=\"stat-value\" style=\"color:%s\">%d</div>", rating_to_color(stats->maxRating), stats->maxRating);
    fprintf(fp, "<div class=\"stat-label\">最高 Rating</div></div>\n");

    fprintf(fp, "<div class=\"stat-card\" style=\"border-color:#3498db\">");
    fprintf(fp, "<div class=\"stat-value\">%d</div>", stats->contestCount);
    fprintf(fp, "<div class=\"stat-label\">总比赛次数</div></div>\n");

    fprintf(fp, "<div class=\"stat-card\" style=\"border-color:#3498db\">");
    fprintf(fp, "<div class=\"stat-value\">%d</div>", stats->recent180Count);
    fprintf(fp, "<div class=\"stat-label\">近 180 天比赛次数</div></div>\n");

    fprintf(fp, "<div class=\"stat-card\" style=\"border-color:%s\">", rating_to_color(stats->recent180MaxRating));
    fprintf(fp, "<div class=\"stat-value\" style=\"color:%s\">%d</div>", rating_to_color(stats->recent180MaxRating), stats->recent180MaxRating);
    fprintf(fp, "<div class=\"stat-label\">近 180 天最高 Rating</div></div>\n");

    fprintf(fp, "<div class=\"stat-card\" style=\"border-color:#2ecc71\">");
    fprintf(fp, "<div class=\"stat-value\">%d</div>", stats->histAll.totalSolved);
    fprintf(fp, "<div class=\"stat-label\">总通过题数 (去重)</div></div>\n");

    fprintf(fp, "</div>\n");

    /* ===== Rating 历史趋势图 ===== */
    fprintf(fp, "<div class=\"section\"><h2>Rating 变化趋势</h2>\n");
    fprintf(fp, "<div class=\"chart-container\"><div id=\"ratingChart\" class=\"chart\"></div></div></div>\n");

    /* ===== 比赛历史表格 ===== */
    fprintf(fp, "<div class=\"section\"><h2>比赛历史</h2>\n");
    fprintf(fp, "<div style=\"overflow-x:auto\">\n<table>\n<thead><tr>"
            "<th>#</th><th>赛事名称</th><th>日期</th>"
            "<th>赛前 Rating</th><th>赛后 Rating</th><th>变化</th>"
            "<th>排名</th><th>现场通过</th><th>赛后补题</th></tr></thead>\n<tbody>\n");

    for (int i = 0; i < stats->totalContests; i++) {
        Contest_Summary *c = &stats->contests[i];
        int delta = c->newRating - c->oldRating;
        const char *delta_class = delta >= 0 ? "rate-up" : "rate-down";
        char delta_str[16];
        snprintf(delta_str, sizeof(delta_str), "%+d", delta);

        fprintf(fp, "<tr>");
        fprintf(fp, "<td>%d</td>", i + 1);
        fprintf(fp, "<td style=\"max-width:300px;overflow:hidden;text-overflow:ellipsis\">%s</td>", c->contestName);
        fprintf(fp, "<td>%s</td>", time_to_date_str(c->timeSeconds));
        fprintf(fp, "<td style=\"color:%s\">%d</td>", rating_to_color(c->oldRating), c->oldRating);
        fprintf(fp, "<td style=\"color:%s\">%d</td>", rating_to_color(c->newRating), c->newRating);
        fprintf(fp, "<td class=\"%s\" style=\"font-weight:bold\">%s</td>", delta_class, delta_str);
        fprintf(fp, "<td>%d</td>", c->rank);

        fprintf(fp, "<td>");
        for (int j = 0; j < c->solvedInContestCount; j++)
            fprintf(fp, "<span class=\"badge badge-ok\">%s</span> ", c->solvedInContest[j]);
        if (c->solvedInContestCount == 0) fprintf(fp, "-");
        fprintf(fp, "</td>");

        fprintf(fp, "<td>");
        for (int j = 0; j < c->upsolvedCount; j++)
            fprintf(fp, "<span class=\"badge badge-up\">%s</span> ", c->upsolved[j]);
        if (c->upsolvedCount == 0) fprintf(fp, "-");
        fprintf(fp, "</td>");

        fprintf(fp, "</tr>\n");
    }
    fprintf(fp, "</tbody></table></div></div>\n");

    /* ===== 题目难度直方图（4 个时间段） ===== */
    fprintf(fp, "<div class=\"section\"><h2>通过题目难度分布</h2>\n");
    fprintf(fp, "<div class=\"hist-grid\">\n");
    fprintf(fp, "<div class=\"chart-container\"><h3 style=\"text-align:center;color:#aaa;margin-bottom:8px\">全部 (%d题)</h3>"
            "<div id=\"histAll\" class=\"hist-chart\"></div></div>\n", stats->histAll.totalSolved);
    fprintf(fp, "<div class=\"chart-container\"><h3 style=\"text-align:center;color:#aaa;margin-bottom:8px\">最近一年 (%d题)</h3>"
            "<div id=\"histYear\" class=\"hist-chart\"></div></div>\n", stats->histYear.totalSolved);
    fprintf(fp, "<div class=\"chart-container\"><h3 style=\"text-align:center;color:#aaa;margin-bottom:8px\">最近180天 (%d题)</h3>"
            "<div id=\"hist180\" class=\"hist-chart\"></div></div>\n", stats->hist180.totalSolved);
    fprintf(fp, "<div class=\"chart-container\"><h3 style=\"text-align:center;color:#aaa;margin-bottom:8px\">最近30天 (%d题)</h3>"
            "<div id=\"hist30\" class=\"hist-chart\"></div></div>\n", stats->hist30.totalSolved);
    fprintf(fp, "</div></div>\n");

    /* ===== eChart 脚本 ===== */
    fputs("<script>\n", fp);

    /* Rating 趋势 */
    fputs("(function(){\nvar dom=document.getElementById('ratingChart');\n"
          "var myChart=echarts.init(dom);\n"
          "var data=[", fp);
    for (int i = stats->totalContests - 1; i >= 0; i--) {
        if (i < stats->totalContests - 1) fputc(',', fp);
        fprintf(fp, "['%s',%d]", time_to_date_str(stats->contests[i].timeSeconds), stats->contests[i].newRating);
    }
    fputs("];\n", fp);

    fputs("var option={\n"
          "tooltip:{trigger:'axis',formatter:function(p){return p[0].axisValue+'<br/>Rating: '+p[0].data[1];}},\n"
          "grid:{left:60,right:30,top:30,bottom:60},\n"
          "xAxis:{type:'time',axisLabel:{color:'#aaa',rotate:30}},\n"
          "yAxis:{type:'value',axisLabel:{color:'#aaa'},splitLine:{lineStyle:{color:'#333'}}},\n"
          "series:[{type:'line',data:data,smooth:true,"
          "lineStyle:{color:'#e94560',width:2},"
          "areaStyle:{color:new echarts.graphic.LinearGradient(0,0,0,1,["
          "{offset:0,color:'rgba(233,69,96,0.5)'},{offset:1,color:'rgba(233,69,96,0.05)'}])},"
          "symbol:'none'}]\n};"
          "myChart.setOption(option);\n"
          "window.addEventListener('resize',function(){myChart.resize();});\n"
          "})();\n", fp);

    /* 难度直方图辅助函数 */
    fputs("function makeHist(id,xData,yData){var dom=document.getElementById(id);"
          "var myChart=echarts.init(dom);"
          "myChart.setOption({"
          "tooltip:{trigger:'axis'},"
          "grid:{left:50,right:20,top:20,bottom:70},"
          "xAxis:{type:'category',data:xData,axisLabel:{color:'#aaa',rotate:45,fontSize:9}},"
          "yAxis:{type:'value',axisLabel:{color:'#aaa'},splitLine:{lineStyle:{color:'#333'}}},"
          "series:[{type:'bar',data:yData,"
          "itemStyle:{color:new echarts.graphic.LinearGradient(0,0,0,1,["
          "{offset:0,color:'#e94560'},{offset:1,color:'#0f3460'}])},barMaxWidth:35}]});"
          "window.addEventListener('resize',function(){myChart.resize();});"
          "return myChart;}\n", fp);

    const char *labels[] = {"histAll","histYear","hist180","hist30"};
    const Problem_Histogram *hists[] = {&stats->histAll, &stats->histYear, &stats->hist180, &stats->hist30};

    for (int h = 0; h < 4; h++) {
        fprintf(fp, "(function(){\nvar xData=[");
        for (int i = 0; i < hists[h]->bucketCount; i++) {
            if (i > 0) fputc(',', fp);
            fprintf(fp, "'%d-%d'", hists[h]->buckets[i].from, hists[h]->buckets[i].to);
        }
        fputs("];\nvar yData=[", fp);
        for (int i = 0; i < hists[h]->bucketCount; i++) {
            if (i > 0) fputc(',', fp);
            fprintf(fp, "%d", hists[h]->buckets[i].count);
        }
        fprintf(fp, "];\nmakeHist('%s',xData,yData);\n})();\n", labels[h]);
    }

    fputs("</script>\n</body>\n</html>", fp);

    fclose(fp);
    printf("HTML 报告已生成: %s\n", output_path);
    return 0;
}
