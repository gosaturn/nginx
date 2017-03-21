#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>
/*
1. 定义ngx_module_t结构体中的commands成员数组
*/
static ngx_int_t ngx_http_gosaturn_handler(ngx_http_request_t *r)
{
    //请求的方法必须是GET或者HEAD
    if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
        return NGX_HTTP_NOT_ALLOWED;
    //丢弃请求中的包体
    ngx_int_t rc = ngx_http_discard_request_body(r);
    if (rc != NGX_OK) 
        return rc;
    ngx_str_t type = ngx_string("text/plain");
    ngx_str_t response = ngx_string("Hello Gosaturn! Go on Fighting!");
    //ngx_write_stdout("test!!!!!!!");
    //ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, r->headers);
    /*
    * 遍历header中的内容
    * */
    ngx_list_part_t* part = &r->headers_in.headers.part; //headers_in中的headers是ngx_list_t类型，其中part指向数组列表中的第一个数组 add by gosaturn
    ngx_table_elt_t* header = part->elts; //数组的第一个元素, header里面存的是key-value类型
    ngx_uint_t i = 0;
    for (i = 0; /*void*/; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            header = part->elts;
            i = 0;
        }
        //printf("list element: %s, %s\n", header[i].key.data, header[i].value.data);
        //不知道怎么打出来，只好先用ngx_log_error
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "header_key=%s;", header[i].key.data);
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "header_value=%s;", header[i].value.data);
    }

    //响应头部
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = response.len;
    r->headers_out.content_type = type;
    //发送http头部
    rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
        return rc;
    
    ngx_buf_t *b = ngx_create_temp_buf(r->pool, response.len);
    if (b == NULL) 
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    ngx_memcpy(b->pos, response.data, response.len);
    b->last = b->pos + response.len;
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;
    
    return ngx_http_output_filter(r, &out);
}
static char* ngx_http_gosaturn(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_gosaturn_handler;
    return NGX_CONF_OK;
};
static ngx_command_t ngx_http_gosaturn_commands[] = {
    {
        ngx_string("gosaturn"), //配置项名
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
        ngx_http_gosaturn, //解析配置项
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};


/*
2. 定义ngx_module_t结构体中的ctx成员
*/
static ngx_http_module_t ngx_http_gosaturn_module_ctx = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

/*
3. 定义ngx_http_gosaturn_module
*/
ngx_module_t ngx_http_gosaturn_module = {
    NGX_MODULE_V1,
    &ngx_http_gosaturn_module_ctx, //请求上下文
    ngx_http_gosaturn_commands,   //commands成员
    NGX_HTTP_MODULE,   //模块类型
    NULL, //init master
    NULL, //init module 
    NULL, //init process 
    NULL, //init thread 
    NULL, //exit thread 
    NULL, //exit process 
    NULL, //exit master 
    NGX_MODULE_V1_PADDING,
};

