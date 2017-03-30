#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_log.h>
#include <ngx_sha1.h>
#include <assert.h>
const ngx_str_t LCS_HEADER_SEC_WEBSOCKET_KEY = ngx_string("Sec-WebSocket-Key");
const ngx_str_t LCS_HEADER_SEC_WEBSOCKET_ACCEPT = ngx_string("Sec-WebSocket-Accept");
const ngx_str_t LCS_HEADER_UPGRADE = ngx_string("Upgrade");
const ngx_str_t LCS_WEBSOCKET = ngx_string("websocket");
const ngx_str_t LCS_HTTP_STATUS_101 = ngx_string("101 Switching Protocols");
// 设置http header_out的headers
ngx_table_elt_t * lcs_add_response_header(ngx_http_request_t *r, const ngx_str_t *header_name, const ngx_str_t *header_value) {
    ngx_table_elt_t *h = ngx_list_push(&r->headers_out.headers);
    if (NULL == h) {
        return NULL;
    }
    h->hash = 1;
    h->key.len = header_name->len;
    h->key.data = header_name->data;
    if(header_value) {
        h->value.len = header_value->len;
        h->value.data = header_value->data;
    } else {
        h->value.len = 0;
        h->value.data = NULL;
    }
    return h;
}

ngx_str_t *lcs_get_header_value(ngx_http_request_t *r, ngx_str_t header_name) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "lcs_get_header_value01");
    ngx_uint_t i;
    ngx_list_part_t *part = &r->headers_in.headers.part;
    ngx_table_elt_t *header = part->elts;
    
    for (i = 0; /* void */; i++) {
        if (i >= part->nelts) {
            if (NULL == part->next) {
                break;
            }
            part = part->next;
            header = part->elts;
            i = 0;
        }
        if (header[i].key.len == header_name.len && ngx_strncasecmp(header[i].key.data, header_name.data, header[i].key.len) == 0) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "lcs_get_header_value02");
            return &header[i].value;
        }
    }
    return NULL;
}

static ngx_int_t ngx_http_lcs_handler(ngx_http_request_t *r)
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
    
    /*
    * 遍历header中的内容
    * */
    ngx_list_part_t* part = &r->headers_in.headers.part; //headers_in中的headers是ngx_list_t类型，其中part指向数组列表中的第一个数组 add by lcs
    ngx_table_elt_t* header = part->elts; //数组的第一个元素, header里面存的是key-value类型
    ngx_uint_t i = 0;
    ngx_uint_t is_connection_upgrade = 0;
    ngx_uint_t is_upgrade_ws = 0;
    static ngx_str_t ws_prefix = ngx_string("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    
    //ngx_str_set(&ws_prefix, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    ngx_uint_t ws_key_len = 0;
    
    for (i = 0; /*void*/; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            header = part->elts;
            i = 0;
        }
        //据说hash=0表示不是合法的头部
        if (0 == header[i].hash) {
            continue;
        }
        if (0 == ngx_strcmp(header[i].key.data, "Connection")) {
            if (0 == ngx_strcmp(header[i].value.data, "Upgrade")) {
                is_connection_upgrade = 1;
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "websocket_header_key=%s;", header[i].key.data);
            }
        }
        if (0 == ngx_strcmp(header[i].key.data, (u_char*) "Upgrade")) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "websocket_header_key=%s;", header[i].key.data);
            if (0 == ngx_strcmp(header[i].value.data, (u_char*) "websocket")) {
                is_upgrade_ws = 1;
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "websocket_header_value=%s;", header[i].value.data);
            }
        }
        if (0 == ngx_strcmp(header[i].key.data, "Sec-WebSocket-Key") && header[i].value.len > 0) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "test01");
            ngx_sha1_t sha1;
            u_char buf_sha1[21];
            u_char buf[255];
            ngx_str_t ws_accept_key; //base64加密后的值
            ngx_str_t sha1_str;
            ngx_str_t *ws_key;
            ws_accept_key.data = buf;
            ws_key_len = sizeof(ws_prefix) +  header[i].value.len;
            
            if (NULL == (ws_key = lcs_get_header_value(r, LCS_HEADER_SEC_WEBSOCKET_KEY))) {
                r->headers_out.status = NGX_HTTP_BAD_REQUEST;
            }

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "test02");
            ngx_sha1_init(&sha1);
            ngx_sha1_update(&sha1, ws_key->data, ws_key->len);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ws_accept_key01=%s", ws_key->data);
            ngx_sha1_update(&sha1, ws_prefix.data, ws_prefix.len);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ws_accept_key02=%s", ws_prefix.data);
            ngx_sha1_final(buf_sha1, &sha1);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ws_accept_key03=%s", buf_sha1);
            sha1_str.len = 20;
            sha1_str.data = buf_sha1;
            ws_accept_key.len = ngx_base64_encoded_length(sha1_str.len);
            assert(ws_accept_key.len < 255);
            ngx_encode_base64(&ws_accept_key, &sha1_str);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ws_accept_key04=%s", ws_accept_key.data);
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "ws_accept_key05=%p", &sha1_str);
            lcs_add_response_header(r, &LCS_HEADER_SEC_WEBSOCKET_ACCEPT, &ws_accept_key);
            lcs_add_response_header(r, &LCS_HEADER_UPGRADE, &LCS_WEBSOCKET);
        }
    }
    //响应头部
    r->headers_out.status = NGX_HTTP_OK;
    // 是websocket header
    if (1 == is_connection_upgrade && 1 == is_upgrade_ws && ws_key_len > 0) {
        r->headers_out.status_line = LCS_HTTP_STATUS_101;
        r->headers_out.status = NGX_HTTP_SWITCHING_PROTOCOLS;
        r->keepalive = 0;
    }
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
static char* ngx_http_lcs(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_lcs_handler;
    return NGX_CONF_OK;
};
/*
1. 定义ngx_module_t结构体中的commands成员数组
*/
static ngx_command_t ngx_http_lcs_commands[] = {
    {
        ngx_string("lcs"), //配置项名
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
        ngx_http_lcs, //解析配置项
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};


/*
2. 定义ngx_module_t结构体中的ctx成员
*/
static ngx_http_module_t ngx_http_lcs_module_ctx = {
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
3. 定义ngx_http_lcs_module
*/
ngx_module_t ngx_http_lcs_module = {
    NGX_MODULE_V1,
    &ngx_http_lcs_module_ctx, //请求上下文
    ngx_http_lcs_commands,   //commands成员
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

