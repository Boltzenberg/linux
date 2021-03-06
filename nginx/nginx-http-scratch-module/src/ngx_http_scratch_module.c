/*
 * ./configure --add-module=/path/to/nginx_http_scratch_module.c
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

// configuration for scratch module
typedef struct {
    ngx_str_t echodata;
} ngx_http_scratch_loc_conf_t;

// Forward declarations
static char *ngx_http_scratch(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_scratch_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_scratch_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_scratch_init(ngx_conf_t *cf);

static ngx_int_t ngx_http_scratch_init_master(ngx_log_t *log);
static ngx_int_t ngx_http_scratch_init_module(ngx_cycle_t *cycle);
static ngx_int_t ngx_http_scratch_init_process(ngx_cycle_t *cycle);
static void ngx_http_scratch_exit_process(ngx_cycle_t *cycle);
static void ngx_http_scratch_exit_master(ngx_cycle_t *cycle);

// Command configuration
static ngx_command_t ngx_http_scratch_commands[] = {
    {
        ngx_string("scratch"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
        ngx_http_scratch,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_scratch_loc_conf_t, echodata),
        NULL,
    },
    ngx_null_command,
};

// Module context
static ngx_http_module_t ngx_http_scratch_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_scratch_init,                 /* postconfiguration */
    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */
    ngx_http_scratch_create_loc_conf,      /* create location configration */
    ngx_http_scratch_merge_loc_conf        /* merge location configration */
};

// Module definition
ngx_module_t ngx_http_scratch_module = {
    NGX_MODULE_V1,
    &ngx_http_scratch_module_ctx,          /* module context */
    ngx_http_scratch_commands,             /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    ngx_http_scratch_init_master,          /* init master */
    ngx_http_scratch_init_module,          /* init module */
    ngx_http_scratch_init_process,         /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ngx_http_scratch_exit_process,         /* exit process */
    ngx_http_scratch_exit_master,          /* exit master */
    NGX_MODULE_V1_PADDING
};

// Handler method
static ngx_int_t
ngx_http_scratch_handler(ngx_http_request_t *r)
{
    ngx_int_t rc;
    ngx_buf_t *b;
    ngx_chain_t out;
    ngx_http_scratch_loc_conf_t *configElement;

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
        "Scratch called to handle request");

    configElement = ngx_http_get_module_loc_conf(r, ngx_http_scratch_module);

    if (!(r->method & (NGX_HTTP_HEAD|NGX_HTTP_GET|NGX_HTTP_POST)))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }

    r->headers_out.content_type.len= sizeof("text/html") - 1;
    r->headers_out.content_type.data = (u_char *) "text/html";
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = configElement->echodata.len;
    if (r->method == NGX_HTTP_HEAD)
    {
        rc = ngx_http_send_header(r);
        if(rc != NGX_OK)
        {
            return rc;
        }
    }

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Failed to allocate response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    out.buf = b;
    out.next = NULL;
    b->pos = configElement->echodata.data;
    b->last = configElement->echodata.data + (configElement->echodata.len);
    b->memory = 1;
    b->last_buf = 1;
    rc = ngx_http_send_header(r);
    if(rc != NGX_OK)
    {
        return rc;
    }

    return ngx_http_output_filter(r, &out);
}

// Creating a new instance of the module
static char *
ngx_http_scratch(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0, "Scratch initialized on config read");
    
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_scratch_handler;
    ngx_conf_set_str_slot(cf,cmd,conf);
    return NGX_CONF_OK;
}

// Creating a new config object
static void *
ngx_http_scratch_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_scratch_loc_conf_t *conf;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0, "Scratch called to create a config object");

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_scratch_loc_conf_t));
    if (conf == NULL)
    {
        return NGX_CONF_ERROR;
    }
    
    conf->echodata.len = 0;
    conf->echodata.data = NULL;
    return conf;
}

// Merge config function
static char *
ngx_http_scratch_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_scratch_loc_conf_t *prev = parent;
    ngx_http_scratch_loc_conf_t *conf = child;

    ngx_log_error(NGX_LOG_INFO, cf->log, 0, "Scratch called to merge a config object");
    
    ngx_conf_merge_str_value(conf->echodata, prev->echodata, NULL);
    return NGX_CONF_OK;
}

// Initialize the module
static ngx_int_t
ngx_http_scratch_init(ngx_conf_t *cf)
{
    ngx_log_error(NGX_LOG_INFO, cf->log, 0, "Scratch initialized post config");
    return NGX_OK;
}

static ngx_int_t 
ngx_http_scratch_init_master(ngx_log_t *log)
{
    ngx_log_error(NGX_LOG_INFO, log, 0, "Scratch called for init master");
    return NGX_OK;
}

static ngx_int_t 
ngx_http_scratch_init_module(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "Scratch called for init module");
    return NGX_OK;
}

static ngx_int_t 
ngx_http_scratch_init_process(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "Scratch called for init process");
    return NGX_OK;
}

static void 
ngx_http_scratch_exit_process(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "Scratch called for exit process");
}

static void 
ngx_http_scratch_exit_master(ngx_cycle_t *cycle)
{
    ngx_log_error(NGX_LOG_INFO, cycle->log, 0, "Scratch called for exit master");
}

























