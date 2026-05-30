#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl.h>

/* libcurl 写回调：将响应数据追加到动态字符串 */
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
    size_t total_size = size * nmemb;
    char **response = (char **)userp;

    if (*response == NULL) {
        *response = (char *)malloc(total_size + 1);
        if (*response == NULL) {
            printf("内存分配失败！\n");
            return 0;
        }
        memcpy(*response, ptr, total_size);
        (*response)[total_size] = '\0';
    } else {
        size_t current_len = strlen(*response);
        char *new_response = (char *)realloc(*response, current_len + total_size + 1);
        if (new_response == NULL) {
            printf("内存分配失败！\n");
            return 0;
        }
        *response = new_response;
        memcpy(*response + current_len, ptr, total_size);
        (*response)[current_len + total_size] = '\0';
    }

    return total_size;
}

/* libcurl 写回调：将响应数据写入文件 */
size_t write_callback_to_file(char *ptr, size_t size, size_t nmemb, void *userp)
{
    FILE *fp = (FILE *)userp;
    return fwrite(ptr, size, nmemb, fp);
}

/* 调用 user.info API，获取用户基本信息 */
void cf_user_get(const char *user_id, char **response) {
    *response = NULL;
    CURL *curl = curl_easy_init();
    CURLcode res;
    if (!curl) {
        printf("curl init failed!\n");
        return;
    }

    char url[256];
    snprintf(url, sizeof(url), "https://codeforces.com/api/user.info?handles=%s", user_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "user.info 请求失败: %s\n", curl_easy_strerror(res));
        free(*response);
        *response = NULL;
    }

    curl_easy_cleanup(curl);
}

/* 调用 user.status API，将提交记录保存到 <user_id>_status.json */
int cf_status_get(const char *user_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s_status.json", user_id);
    FILE *fp = fopen(file_path, "w");
    if (!fp) {
        printf("文件创建失败！\n");
        return -1;
    }
    CURL *curl = curl_easy_init();
    if (!curl) {
        printf("curl init failed!\n");
        fclose(fp);
        return -1;
    }
    char url[256];
    snprintf(url, sizeof(url), "https://codeforces.com/api/user.status?handle=%s", user_id);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "user.status 请求失败: %s\n", curl_easy_strerror(res));
        return -1;
    }
    return 0;
}

/* 调用 user.rating API，将等级分历史保存到 <user_id>_rating.json */
int cf_rating_get(const char *user_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s_rating.json", user_id);
    FILE *fp = fopen(file_path, "w");
    if (!fp) {
        printf("文件创建失败！\n");
        return -1;
    }
    CURL *curl = curl_easy_init();
    if (!curl) {
        printf("curl init failed!\n");
        fclose(fp);
        return -1;
    }

    char url[256];
    snprintf(url, sizeof(url), "https://codeforces.com/api/user.rating?handle=%s", user_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "user.rating 请求失败: %s\n", curl_easy_strerror(res));
        return -1;
    }
    return 0;
}
