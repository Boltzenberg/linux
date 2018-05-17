#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

// Forward declarations
static char *ngx_http_upstream_setup_aslb(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static ngx_int_t ngx_http_upstream_init_aslb(ngx_conf_t *cf, 
    ngx_http_upstream_srv_conf_t *us);
static ngx_int_t ngx_http_upstream_init_aslb_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);
static ngx_int_t ngx_http_upstream_get_aslb_peer(ngx_peer_connection_t *pc,
    void *data);
void ngx_http_upstream_free_aslb_peer(ngx_peer_connection_t *pc,
    void *data, ngx_uint_t state);

// Data structs for handling requests
typedef struct {
    struct sockaddr               *sockaddr;
    socklen_t                      socklen;
    ngx_str_t                      name;
    ngx_str_t                      server;
} ngx_http_upstream_aslb_peer_t;

typedef struct {
    ngx_uint_t                     peerCount;
    ngx_http_upstream_aslb_peer_t *peerArray;
} ngx_http_upstream_aslb_data_t;

// Module registration
static ngx_command_t ngx_http_upstream_aslb_commands[] = {

    { ngx_string("aslb"),
      NGX_HTTP_UPS_CONF|NGX_CONF_NOARGS,
      ngx_http_upstream_setup_aslb,
      0,
      0,
      NULL },

      ngx_null_command
};

static ngx_http_module_t ngx_http_upstream_aslb_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */
    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */
    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

ngx_module_t ngx_http_upstream_aslb_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_aslb_module_ctx,    /* module context */
    ngx_http_upstream_aslb_commands,       /* module directives */
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

// When the aslb directive is encountered in config (valid in the upstream block),
// this is called to register the init_upstream callback and define the valid
// modifiers to the load balancer selection.
static char *
ngx_http_upstream_setup_aslb(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t  *uscf;

    // ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "JONRO: ngx_http_upstream_aslb_setup");

    uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);

    // Make sure we're the only load balancer intialized so far.
    if (uscf->peer.init_upstream) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "load balancing method redefined");
    }

    // Register the upstream initialization function.
    uscf->peer.init_upstream = ngx_http_upstream_init_aslb;

    // Register the valid modifiers for the ASLB load balancer
    uscf->flags = NGX_HTTP_UPSTREAM_CREATE;

    return NGX_CONF_OK;
}

// When configuration is loaded, this method initializes the group
// of servers in the upstream being handled by ASLB.
static ngx_int_t
ngx_http_upstream_init_aslb(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
{
    ngx_http_upstream_aslb_data_t *data;
    ngx_http_upstream_aslb_peer_t *peerArray;
    ngx_http_upstream_server_t    *server;
    ngx_uint_t                     cAddr;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "JONRO: aslb peer initialization");

    // We need there to be servers in the upstream config block
    if (us->servers == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
            "ASLB is only valid for upstream servers");
        return NGX_ERROR;
    }

    // Count the number of addresses.  There may be many per server (v4 and v6)
    server = us->servers->elts;
    cAddr = 0;
    for (ngx_uint_t i = 0; i < us->servers->nelts; i++) {
        cAddr += server[i].naddrs;
    }

    // If we didn't find any addresses, that's trouble.
    if (cAddr == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, 
            "No servers in upstream \"%V\" in %s:%ui",
            &us->host, us->file_name, us->line);
        return NGX_ERROR;
    }

    // Allocate the memory for the array of addresses and supporting info
    peerArray = ngx_pcalloc(cf->pool, 
        sizeof(ngx_http_upstream_aslb_peer_t) * cAddr);

    if (peerArray == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, 
            "OOM creating the peer array");
        return NGX_ERROR;
    }

    // Loop through the servers, adding all addresses to the peer array
    for (ngx_uint_t iPeer = 0, iServer = 0;
         iServer < us->servers->nelts;
         iServer++) {
        for (ngx_uint_t iServerAddr = 0;
             iServerAddr < server[iServer].naddrs;
             iServerAddr++) {
            peerArray[iPeer].sockaddr = server[iServer].addrs[iServerAddr].sockaddr;
            peerArray[iPeer].socklen = server[iServer].addrs[iServerAddr].socklen;
            peerArray[iPeer].name = server[iServer].addrs[iServerAddr].name;
            peerArray[iPeer].server = server[iServer].name;
            iPeer++;
        }
    }

    // Store the whole thing on the ASLB data object
    data = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_aslb_data_t));
    if (data == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "OOM creating the ASLB data");
        return NGX_ERROR;
    }

    data->peerCount = cAddr;
    data->peerArray = peerArray;

    // Initialize the upstream peer configuration.
    us->peer.data = data;
    us->peer.init = ngx_http_upstream_init_aslb_peer;

    return NGX_OK;
}

// When a request comes in, this method gets all of the data needed
// to successfully select an upstream server for the request.
// Set the r->upstream->peer.data, tries, get and free in here.
static ngx_int_t
ngx_http_upstream_init_aslb_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us)
{
    ngx_http_upstream_aslb_data_t *aslbData = us->peer.data;

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, 
        "JONRO: ngx_http_upstream_init_aslb_peer");
    
    // Combine the ASLB health data with the servers configured
    // in this upstream pool.

    // Set tries to the number of peers and count down from there.
    r->upstream->peer.data = aslbData;
    r->upstream->peer.tries = aslbData->peerCount;
    r->upstream->peer.get = ngx_http_upstream_get_aslb_peer;
    r->upstream->peer.free = ngx_http_upstream_free_aslb_peer;

    return NGX_OK;
}

// Actually make the selection of which upstream server to use.
// Fill in the sockaddr, socklen, and name fields of pc.  data
// is what was created in ngx_http_upstream_init_aslb_peer.
// Set pc->sockaddr, socklen, name in here.
static ngx_int_t
ngx_http_upstream_get_aslb_peer(ngx_peer_connection_t *pc, void *data)
{
    // peerIndex assumes the aslbData peerArray is sorted from "best" to
    // "worst" per ASLB algorithms.  It uses the pc->tries value to keep
    // track of where in the array we are in terms of retries.  When 
    // pc->tries == 0, nginx stops retrying, so we count down the remaining
    // retries and turn that number in to the current index.
    ngx_http_upstream_aslb_data_t *aslbData = data;
    ngx_uint_t peerIndex = aslbData->peerCount - pc->tries;

    ngx_log_error(NGX_LOG_INFO, pc->log, 0, 
        "JONRO: ngx_http_upstream_get_aslb_peer: (try %ui of %ui)",
        peerIndex + 1, aslbData->peerCount);
    
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "get aslb peer, try: %ui", pc->tries);

    if (peerIndex == aslbData->peerCount) {
        ngx_log_error(NGX_LOG_INFO, pc->log, 0, 
            "Out of options for peers to send to!");
        return NGX_ERROR;
    }

    ngx_log_error(NGX_LOG_INFO, pc->log, 0, 
        "selecting \"%V\" from \"%V\" in iteration %ui",
        &aslbData->peerArray[peerIndex].name,
        &aslbData->peerArray[peerIndex].server,
        peerIndex);

    pc->sockaddr = aslbData->peerArray[peerIndex].sockaddr;
    pc->socklen = aslbData->peerArray[peerIndex].socklen;
    pc->name = &aslbData->peerArray[peerIndex].name;

    return NGX_OK;
}

// Called once per call to ngx_http_upstream_get_aslb_peer to clean up
// resources and do bookkeeping.  This can trigger retries by keeping
// pc->tries > 0.  state indicates if the connection to the peer returned
// by get_aslb_peer succeeded and/or if the response was success or failure.
void
ngx_http_upstream_free_aslb_peer(
    ngx_peer_connection_t *pc,
    void *data,
    ngx_uint_t state)
{
    ngx_http_upstream_aslb_data_t *aslbData = data;
    ngx_uint_t peerIndex = aslbData->peerCount - pc->tries;

    ngx_log_error(NGX_LOG_INFO, pc->log, 0, 
        "JONRO: ngx_http_upstream_free_aslb_peer (try %ui of %ui): %xi",
        peerIndex + 1, aslbData->peerCount, state);
    
    // Retry on connection failure or bail otherwise.
    if ((state & NGX_PEER_FAILED) == NGX_PEER_FAILED) {
        pc->tries--;
    } else {
        pc->tries = 0;
    }
}