#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/bufferevent.h>

static void http_request_done(struct evhttp_request *req, void *arg){
    char buf[1024];

    // req is null when there is timeout
    if (req == NULL) {
        printf("request is null, maybe timeout\n");
        return;
    }
    int s = evbuffer_remove(req->input_buffer, &buf, sizeof(buf) - 1);
    buf[s] = '\0';
    printf("%s", buf);
    // terminate event_base_dispatch()
    event_base_loopbreak((struct event_base *)arg);
}

static int http_send_pos(std::string *string){
    struct event_base *base;
    struct evhttp_connection *conn = NULL;
    struct evhttp_request *req;

    base = event_base_new();
    conn = evhttp_connection_base_new(base, NULL, "127.0.0.1", 8668);
    req = evhttp_request_new(http_request_done, base);
    //   req->WriteHeader("Content-Type", "application/octet-stream");
    //   req->WriteReply(HTTP_OK, binaryBlock);

    evhttp_add_header(req->output_headers, "Host", "localhost");
    struct evbuffer* evb = evhttp_request_get_output_buffer(req);
    assert(evb);
    evbuffer_add(evb, string->data(), string->size());
    std::string endpoint = "/pow";

    evhttp_make_request(conn, req, EVHTTP_REQ_POST, endpoint.c_str());
    evhttp_connection_set_timeout(req->evcon, 600);
    event_base_dispatch(base);

    if (conn != NULL) evhttp_connection_free(conn);
    if (base != NULL) event_base_free(base);
    return 0;
}

