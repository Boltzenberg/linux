#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    /* the round robin data must be first */
    ngx_http_upstream_rr_peer_data_t   rrp;

    ngx_uint_t                         hash;

    u_char                             addrlen;
    u_char                            *addr;

    u_char                             tries;

    ngx_event_get_peer_pt              get_rr_peer;
} ngx_http_upstream_aslb_peer_data_t;


static ngx_int_t ngx_http_upstream_init_aslb_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);
static ngx_int_t ngx_http_upstream_get_aslb_peer(ngx_peer_connection_t *pc,
    void *data);
static char *ngx_http_upstream_aslb(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_http_upstream_aslb_commands[] = {

    { ngx_string("aslb"),
      NGX_HTTP_UPS_CONF|NGX_CONF_NOARGS,
      ngx_http_upstream_aslb,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_upstream_aslb_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_upstream_aslb_module = {
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


static u_char ngx_http_upstream_aslb_pseudo_addr[3];

static ngx_int_t
ngx_http_upstream_init_aslb(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
{
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "JONRO: aslb peer initialization");
        
    if (ngx_http_upstream_init_round_robin(cf, us) != NGX_OK) {
        return NGX_ERROR;
    }

    us->peer.init = ngx_http_upstream_init_aslb_peer;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_init_aslb_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us)
{
    struct sockaddr_in                     *sin;
#if (NGX_HAVE_INET6)
    struct sockaddr_in6                    *sin6;
#endif
    ngx_http_upstream_aslb_peer_data_t  *aslbp;

    ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "JONRO: aslb 3");
    
    aslbp = ngx_palloc(r->pool, sizeof(ngx_http_upstream_aslb_peer_data_t));
    if (aslbp == NULL) {
        return NGX_ERROR;
    }

    r->upstream->peer.data = &aslbp->rrp;

    if (ngx_http_upstream_init_round_robin_peer(r, us) != NGX_OK) {
        return NGX_ERROR;
    }

    r->upstream->peer.get = ngx_http_upstream_get_aslb_peer;

    switch (r->connection->sockaddr->sa_family) {

    case AF_INET:
        sin = (struct sockaddr_in *) r->connection->sockaddr;
        aslbp->addr = (u_char *) &sin->sin_addr.s_addr;
        aslbp->addrlen = 3;
        break;

#if (NGX_HAVE_INET6)
    case AF_INET6:
        sin6 = (struct sockaddr_in6 *) r->connection->sockaddr;
        aslbp->addr = (u_char *) &sin6->sin6_addr.s6_addr;
        aslbp->addrlen = 16;
        break;
#endif

    default:
        aslbp->addr = ngx_http_upstream_aslb_pseudo_addr;
        aslbp->addrlen = 3;
    }

    aslbp->hash = 89;
    aslbp->tries = 0;
    aslbp->get_rr_peer = ngx_http_upstream_get_round_robin_peer;

    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_get_aslb_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_upstream_aslb_peer_data_t  *aslbp = data;

    time_t                        now;
    ngx_int_t                     w;
    uintptr_t                     m;
    ngx_uint_t                    i, n, p, hash;
    ngx_http_upstream_rr_peer_t  *peer;

    ngx_log_error(NGX_LOG_INFO, pc->log, 0, "JONRO: aslb 4");
    
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "get aslb peer, try: %ui", pc->tries);

    /* TODO: cached */

    ngx_http_upstream_rr_peers_wlock(aslbp->rrp.peers);

    if (aslbp->tries > 20 || aslbp->rrp.peers->single) {
        ngx_http_upstream_rr_peers_unlock(aslbp->rrp.peers);
        return aslbp->get_rr_peer(pc, &aslbp->rrp);
    }

    now = ngx_time();

    pc->cached = 0;
    pc->connection = NULL;

    hash = aslbp->hash;

    for ( ;; ) {

        for (i = 0; i < (ngx_uint_t) aslbp->addrlen; i++) {
            hash = (hash * 113 + aslbp->addr[i]) % 6271;
        }

        w = hash % aslbp->rrp.peers->total_weight;
        peer = aslbp->rrp.peers->peer;
        p = 0;

        while (w >= peer->weight) {
            w -= peer->weight;
            peer = peer->next;
            p++;
        }

        n = p / (8 * sizeof(uintptr_t));
        m = (uintptr_t) 1 << p % (8 * sizeof(uintptr_t));

        if (aslbp->rrp.tried[n] & m) {
            goto next;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "get aslb peer, hash: %ui %04XL", p, (uint64_t) m);

        if (peer->down) {
            goto next;
        }

        if (peer->max_fails
            && peer->fails >= peer->max_fails
            && now - peer->checked <= peer->fail_timeout)
        {
            goto next;
        }

        if (peer->max_conns && peer->conns >= peer->max_conns) {
            goto next;
        }

        break;

    next:

        if (++aslbp->tries > 20) {
            ngx_http_upstream_rr_peers_unlock(aslbp->rrp.peers);
            return aslbp->get_rr_peer(pc, &aslbp->rrp);
        }
    }

    aslbp->rrp.current = peer;

    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;
    pc->name = &peer->name;

    peer->conns++;

    if (now - peer->checked > peer->fail_timeout) {
        peer->checked = now;
    }

    ngx_http_upstream_rr_peers_unlock(aslbp->rrp.peers);

    aslbp->rrp.tried[n] |= m;
    aslbp->hash = hash;

    return NGX_OK;
}


static char *
ngx_http_upstream_aslb(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t  *uscf;

    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "JONRO: aslb registration function");

    uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);

    if (uscf->peer.init_upstream) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "load balancing method redefined");
    }

    uscf->peer.init_upstream = ngx_http_upstream_init_aslb;

    uscf->flags = NGX_HTTP_UPSTREAM_CREATE
                  |NGX_HTTP_UPSTREAM_FAIL_TIMEOUT
                  |NGX_HTTP_UPSTREAM_DOWN
                  |NGX_HTTP_UPSTREAM_BACKUP;

    return NGX_CONF_OK;
}
