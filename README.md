# ngx_http_request_t结构体说明
## ngx_http_headers_in_t headers_in
* 主要保存接收到的http请求头的内容;
* ngx_table_elt_t格式为:

  
```
        typedef struct {
          ngx_uint_t        hash;
          ngx_str_t         key; <!-- key为请求头中的字段 -->
          ngx_str_t         value; <!-- value为对应字段的值 -->
          u_char           *lowcase_key;
        } ngx_table_elt_t;
```
* ngx_str_t格式为:

```
        typedef struct {
            size_t      len; <!-- 指针指向的空间长度 -->
            u_char     *data; <!-- 无符号字符型数组指针 -->
        } ngx_str_t;
```
* 常用的字段有:

```
        authorization
        accept
        content_type
        user_agent
        transfer_encoding
        keep_alive
```
## ngx_http_headers_out_t headers_out
* 主要保存用于返回给客户端的http响应头，其中的字段定义与ngx_http_headers_in_t稍有区别;
## ngx_buf_t* header_in
* 主要保存接收到的http请求body中的内容;

```
        typedef struct ngx_buf_s  ngx_buf_t;
        struct ngx_buf_s {
            u_char          *pos;
            u_char          *last;
            off_t            file_pos;
            off_t            file_last;
            u_char          *start;
            u_char          *end;
            ngx_buf_tag_t    tag;
            ngx_file_t      *file;
            ngx_buf_t       *shadow;
            unsigned         temporary:1;
            unsigned         memory:1;
            unsigned         recycled:1;
            unsigned         in_file:1;
            unsigned         flush:1;
            unsigned         sync:1;
            unsigned         last_buf:1;
            unsigned         last_in_chain:1;
            unsigned         last_shadow:1;
            unsigned         temp_file:1;
            /* STUB */ int   num;
        }
  
```
    struct ngx_buf_s中,常用的:start/pos/last/end/last_buf可能是需要人为修改的,指定到发送缓存ngx_buf_t* b指针中;
## ngx_http_discard_request_body函数
* 丢弃请求中的包体
##







































































