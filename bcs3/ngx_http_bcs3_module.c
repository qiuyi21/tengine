
/*
 * Copyright (C) Bingosoft
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
	ngx_flag_t enabled;
} ngx_http_bcs3_loc_conf_t;


typedef struct {
    ngx_flag_t send500;
} ngx_http_bcs3_ctx_t;


static ngx_int_t ngx_http_bcs3_init(ngx_conf_t *cf);
static void *ngx_http_bcs3_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_bcs3_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);


static ngx_command_t  ngx_http_bcs3_commands[] = {

    { ngx_string("bcs3_filter"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_bcs3_loc_conf_t, enabled),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_bcs3_module_ctx = {
    NULL,                                  /* preconfiguration */
	ngx_http_bcs3_init,                    /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

	ngx_http_bcs3_create_loc_conf,         /* create location configuration */
	ngx_http_bcs3_merge_loc_conf           /* merge location configuration */
};


ngx_module_t  ngx_http_bcs3_module = {
    NGX_MODULE_V1,
    &ngx_http_bcs3_module_ctx,     /* module context */
    ngx_http_bcs3_commands,        /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt   ngx_http_next_body_filter;

static ngx_str_t m_content_type = ngx_string("application/xml");
static ngx_str_t m_err500 = ngx_string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Error><Code>InternalError</Code><Message>Server encountered an internal error.</Message><RequestId/><HostId/></Error>");


static ngx_int_t ngx_http_bcs3_header_filter(ngx_http_request_t *r) {
    ngx_http_bcs3_loc_conf_t *lcf = ngx_http_get_module_loc_conf(r, ngx_http_bcs3_module);
    if (!lcf->enabled) goto out;

    ngx_http_upstream_t *u = r->upstream;
    if (!u) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "qiuyitest: header_filter no upstream");
        goto out;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "qiuyitest: header_filter status_line=\"%V\"", &u->headers_in.status_line);
    if (u->headers_in.status_line.len > 0) goto out;

    ngx_http_bcs3_ctx_t *ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_bcs3_ctx_t));
    if (!ctx) return NGX_ERROR;

    ctx->send500 = 1;
    ngx_http_set_ctx(r, ctx, ngx_http_bcs3_module);

    r->headers_out.content_type_len = m_content_type.len;
    r->headers_out.content_type = m_content_type;
    r->headers_out.content_type_lowcase = NULL;

    r->headers_out.content_length_n = m_err500.len;

    if (r->headers_out.content_length) {
        r->headers_out.content_length->hash = 0;
        r->headers_out.content_length = NULL;
    }

    ngx_http_clear_accept_ranges(r);

out:
    return ngx_http_next_header_filter(r);
}


static ngx_int_t ngx_http_bcs3_body_filter(ngx_http_request_t *r, ngx_chain_t *in) {
    ngx_http_bcs3_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_bcs3_module);
    if (!ctx || !ctx->send500) goto out;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "qiuyitest: body_filter start");

    ngx_buf_t *buf = ngx_calloc_buf(r->pool);
    if (!buf) return NGX_ERROR;

    buf->pos = m_err500.data;
    buf->last = buf->pos + m_err500.len;
    buf->start = buf->pos;
    buf->end = buf->last;
    buf->last_buf = 1;
    buf->memory = 1;

    in->buf = buf;
    in->next = NULL;

out:
    return ngx_http_next_body_filter(r, in);
}


static ngx_int_t ngx_http_bcs3_init(ngx_conf_t *cf) {
    ngx_http_next_body_filter   = ngx_http_top_body_filter;
    ngx_http_top_body_filter    = ngx_http_bcs3_body_filter;

    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter  = ngx_http_bcs3_header_filter;
	return NGX_OK;
}


static void *ngx_http_bcs3_create_loc_conf(ngx_conf_t *cf) {
	ngx_http_bcs3_loc_conf_t *conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_bcs3_loc_conf_t));
	if (conf) {
		conf->enabled = NGX_CONF_UNSET;
	}
	return conf;
}

static char *ngx_http_bcs3_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child) {
	ngx_http_bcs3_loc_conf_t *prev = parent;
	ngx_http_bcs3_loc_conf_t *conf = child;

    ngx_conf_merge_value(conf->enabled, prev->enabled, 0);

	return NGX_CONF_OK;
}
