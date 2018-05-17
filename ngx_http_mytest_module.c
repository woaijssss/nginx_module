#include <string.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <curl/curl.h>
#include <iconv.h>
#include "cJSON.h"

#define BUFSIZE    8192
#define NON_NUM '0'

static size_t response_len = 0;

//------------------------------------------------------------------
static ngx_int_t responseMsg(ngx_http_request_t* r, ngx_str_t* response);
static ngx_http_request_t* getHeader(ngx_http_request_t* r);
static ngx_int_t sendHttpMsg(ngx_http_request_t* r, ngx_str_t* response);
static size_t writeFunc(void *ptr, size_t size, size_t nmemb, void *data);
static void setCurlOps(CURL* curl, const char* resp, const char* method);
/*
  调用联通平台服务器，将token转换成租户名和用户名
 */
static int curlOps(char* resp, const char* method, const char* token);

//------------------------------------------------------------------

static char* ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r);

//处理配置项
static ngx_command_t  ngx_http_mytest_commands[] =
{

        {
                ngx_string("test_command_name"),
                NGX_HTTP_MAIN_CONF
                | NGX_HTTP_SRV_CONF
                | NGX_HTTP_LOC_CONF
                | NGX_HTTP_LMT_CONF
                | NGX_CONF_NOARGS,
                ngx_http_mytest,
                NGX_HTTP_LOC_CONF_OFFSET,
                0,
                NULL
        },

        ngx_null_command
};

//模块上下文
static ngx_http_module_t  ngx_http_mytest_module_ctx =
{
        NULL,                              /* preconfiguration */
        NULL,                       /* postconfiguration */

        NULL,                              /* create main configuration */
        NULL,                              /* init main configuration */

        NULL,                              /* create server configuration */
        NULL,                              /* merge server configuration */

        NULL,                   /* create location configuration */
        NULL                    /* merge location configuration */
};

//新模块定义
ngx_module_t  ngx_http_mytest_module =
{
        NGX_MODULE_V1,
        &ngx_http_mytest_module_ctx,           /* module context */
        ngx_http_mytest_commands,              /* module directives */
        NGX_HTTP_MODULE,                       /* module type */
        NULL,                                  /* init master */
        NULL,                                  /* init module */
        NULL,                                  /* init process */
        NULL,                                  /* init thread */
        NULL,                                  /* exit thread */
        NULL,                                  /* exit process */
        NULL,                                  /* exit master */
        NGX_MODULE_V1_PADDING
};

//配置项对应的回调函数
static char * ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
        ngx_http_core_loc_conf_t  *clcf;

        /*
          首先找到mytest配置项所属的配置块，clcf貌似是location块内的数据
          结构，其实不然，它可以是main、srv或者loc级别配置项，也就是说在每个
          http{}和server{}内也都有一个ngx_http_core_loc_conf_t结构体
        */
        clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

        /*
          http框架在处理用户请求进行到NGX_HTTP_CONTENT_PHASE阶段时，如果
          请求的主机域名、URI与mytest配置项所在的配置块相匹配，就将调用我们
          实现的ngx_http_mytest_handler方法处理这个请求
        */
        clcf->handler = ngx_http_mytest_handler;

        return NGX_CONF_OK;
}

//实际完成处理的回调函数
static ngx_int_t ngx_http_mytest_handler(ngx_http_request_t *r)
{
        //必须是GET或者HEAD方法，否则返回405 Not Allowed
        /* uint16_t m = NGX_HTTP_GET | NGX_HTTP_POST | NGX_HTTP_DELETE | NGX_HTTP_PUT; */
        /* if (!(r->method & m)) { */
        /*         return NGX_HTTP_NOT_ALLOWED; */
        /* } */

        char method[8] = {0};
        switch (r->method) {
        case NGX_HTTP_GET:
                memcpy(method, "GET", 3);
                break;
        case NGX_HTTP_POST:
                memcpy(method, "POST", 4);
                break;
        case NGX_HTTP_DELETE:
                memcpy(method, "DELETE", 6);
                break;
        case NGX_HTTP_PUT:
                memcpy(method, "PUT", 3);
                break;
        default:
                return NGX_HTTP_NOT_ALLOWED;
        }

        char resp[BUFSIZE] = {0};

        int ret = curlOps(resp, "GET", "");
        if (-1 == ret) {
                ret = 0;
        }

        //返回的包体内容
        char buff[128] = "d2VuaGFuOndlbmhhbkAxMjM=";
        u_char* p_sub_str_1 = (u_char*)strstr((char*)r->args.data, "=");
        u_char* p_sub_str_2 = (u_char*)strstr((char*)r->args.data, "&");
        u_char* p = (u_char*)malloc(r->args.len);
        memset(p, 0, r->args.len);
        memcpy(p, p_sub_str_1+1, p_sub_str_2 - p_sub_str_1 - 1);
        char tmp [1024] = {0};

        cJSON* json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "Authorization", buff);
        char* p_buff = cJSON_PrintUnformatted(json);
        snprintf(tmp, r->args.len+strlen(buff)+2, "%s(%s)", p, p_buff);
        free(p);
        p = NULL;

        ngx_str_t response = ngx_string(tmp);
        response.len = strlen(tmp);

        /* ngx_str_t response; */
        /* response.data = (u_char*)cJSON_PrintUnformatted(json); */
        /* response.len = strlen((char*)response.data); */

//        ngx_str_t response = {strlen(buff), (u_char*)buff};
        /* char* p = (char*)r->header_in->pos; */
        /* ngx_str_t response = {strlen(p), (u_char*)p}; */
        //响应包是有包体内容的，所以需要设置Content-Length长度
//        r->headers_out.content_length_n = response.len;

        return responseMsg(r, &response);
}

ngx_http_request_t* getHeader(ngx_http_request_t* r)
{
        //设置返回状态码
        r->headers_out.status = NGX_HTTP_OK;

//        if (r->headers_in.content_type->value.data) {
        /* if (r->headers_in.content_type->value.len) { */
        /*         r->headers_out.content_type = r->headers_in.content_type->value; */
        /* } */
        r->headers_out.content_encoding = r->headers_in.transfer_encoding;
        ngx_str_t cs = ngx_string("utf-8");
        r->headers_out.charset = cs;

        return r;
}

ngx_int_t responseMsg(ngx_http_request_t* r, ngx_str_t* response)
{
        r = getHeader(r);
        return sendHttpMsg(r, response);
}

ngx_int_t sendHttpMsg(ngx_http_request_t* r, ngx_str_t* response)
{
        //发送http头部
        ngx_int_t rc = ngx_http_send_header(r);
        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
        {
                return rc;
        }

        // 构造ngx_buf_t结构准备发送包体
        ngx_buf_t* b;
        b = ngx_create_temp_buf(r->pool, response->len);
        if (b == NULL)
        {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }
        // 将要返回的信息拷贝到ngx_buf_t指向的内存中
        ngx_memcpy(b->pos, response->data, response->len);
        //注意，一定要设置好last指针
        /* b->start = r->header_in->pos; */
        /* b->pos = r->header_in->pos; */
        /* b->last = b->pos + response.len; */
        /* b->end = r->header_in->end; */
        b->start = response->data;
        b->pos = response->data;
        b->last = b->pos + response->len;
        b->end = b->pos + response->len - 1;
        //声明这是最后一块缓冲区
        b->last_buf = 1;

        //构造发送时的ngx_chain_t结构体
        ngx_chain_t out;
        //赋值ngx_buf_t
        out.buf = b;            /* ngx_chain_t */
        //设置next为NULL
        out.next = NULL;

        /*
          最后一步发送包体，http框架会调用ngx_http_finalize_request方法
          结束请求
        */
        return ngx_http_output_filter(r, &out);
}

size_t writeFunc(void *ptr, size_t size, size_t nmemb, void *data)
{
        const size_t n = size * nmemb;
        if (n > 0)
        {
                memcpy((char*)data, (char*)ptr, nmemb);
                response_len = n;
        }

        return n;
}

void setCurlOps(CURL* curl, const char* resp, const char* method)
{
        // 设置http连接自定义端口
        /* curl_easy_setopt(curl, CURLOPT_PORT, 28084); */
//        curl_easy_setopt(curl,CURLOPT_POST,1); //设置问非0表示本次操作为post
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
        curl_easy_setopt(curl, CURLOPT_VERBOSE,0); //打印调试信息
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        /* curl_easy_setopt(curl, CURLOPT_URL, */
        /*                  "http://27.115.50.75/portal/interface/checkToken"); */
        const char* url =
                "http://wenhan.quarkioe.com/inventory/managedObjects/75034265";
        curl_easy_setopt(curl, CURLOPT_URL, url);
//                         "http://27.115.50.75:28084/portal/interface/checkToken");
}

/*
  调用联通平台服务器，将token转换成租户名和用户名
 */
int curlOps(char* resp, const char* method, const char* token)
{
        int ret = -1;
        /* if (!strncmp(method, "POST", sizeof("POST"))) { */
        if (!strncmp(method, "GET", sizeof("GET"))) {
                ret = 0;
                CURL *curl = curl_easy_init();

                setCurlOps(curl, resp, method);

                struct curl_slist *headers = NULL;
                headers = curl_slist_append(
                        headers, "authorization: Basic d2VuaGFuOndlbmhhbkAxMjM="
                        );
                headers = curl_slist_append(headers, "cache-control: no-cache");
                headers = curl_slist_append(headers,
                                            "content-type: application/json");
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                /* char post_body[1024] = {0}; */
                /* snprintf(post_body, sizeof(post_body), */
                /*          "{\n\t\"token\":\"%s\",\n\t\"fromSys\":\"PORTAL\"\n}", */
                /*          token); */

                /* curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body); */

//                CURLcode ret = curl_easy_perform(curl);
                curl_easy_perform(curl);
                curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &ret);

                curl_slist_free_all(headers);//记得要释放
                curl_easy_cleanup(curl);
        }


        return ret;
}
