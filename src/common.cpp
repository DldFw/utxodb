#include <iostream>
#include <string>
#include <fstream>
#include "common.h"
#include <curl/curl.h>
#include <sstream>
#include <glog/logging.h>
#include "support/events.h"
#include <event2/buffer.h>
bool g_node_dump = false;
static size_t ReplyCallback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    std::string *str = (std::string*)stream;
    (*str).append((char*)ptr, size*nmemb);
    return size * nmemb;
}


std::string EncodeBase64(const unsigned char* pch, size_t len)
{
    static const char *pbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string strRet = "";
    strRet.reserve((len+2)/3*4);

    int mode=0, left=0;
    const unsigned char *pchEnd = pch+len;

    while (pch<pchEnd)
    {
        int enc = *(pch++);
        switch (mode)
        {
            case 0: // we have no bits
                strRet += pbase64[enc >> 2];
                left = (enc & 3) << 4;
                mode = 1;
                break;

            case 1: // we have two bits
                strRet += pbase64[left | (enc >> 4)];
                left = (enc & 15) << 2;
                mode = 2;
                break;

            case 2: // we have four bits
                strRet += pbase64[left | (enc >> 6)];
                strRet += pbase64[enc & 63];
                mode = 0;
                break;
        }
    }

    if (mode)
    {
        strRet += pbase64[left];
        strRet += '=';
        if (mode == 1)
            strRet += '=';
    }

    return strRet;
}

std::string EncodeBase64(const std::string& str)
{
    return EncodeBase64((const unsigned char*)str.c_str(), str.size());
}


struct HTTPReply
{
    HTTPReply() : status(0), error(-1) {}

    int status;
    int error;
    std::string body;
};
static const int DEFAULT_HTTP_CLIENT_TIMEOUT = 900;


void http_request_done(struct evhttp_request *req, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply *>(ctx);

    if (req == nullptr)
    {
        /**
         * If req is nullptr, it means an error occurred while connecting: the
         * error code will have been passed to http_error_cb.
         */
        reply->status = 0;
        return;
    }

    reply->status = evhttp_request_get_response_code(req);

    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    if (buf)
    {
        size_t size = evbuffer_get_length(buf);
        const char *data = (const char *)evbuffer_pullup(buf, size);
        if (data) reply->body = std::string(data, size);
        evbuffer_drain(buf, size);
    }
}


const char *http_errorstring(int code)
{
    switch (code)
    {
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
        case EVREQ_HTTP_TIMEOUT:
            return "timeout reached";
        case EVREQ_HTTP_EOF:
            return "EOF reached";
        case EVREQ_HTTP_INVALID_HEADER:
            return "error while reading header, or invalid header";
        case EVREQ_HTTP_BUFFER_ERROR:
            return "error encountered while reading or writing";
        case EVREQ_HTTP_REQUEST_CANCEL:
            return "request was canceled";
        case EVREQ_HTTP_DATA_TOO_LONG:
            return "response body is larger than allowed";
#endif
        default:
            return "unknown";
    }
}


#if LIBEVENT_VERSION_NUMBER >= 0x02010300
static void http_error_cb(enum evhttp_request_error err, void *ctx) {
    HTTPReply *reply = static_cast<HTTPReply *>(ctx);
    reply->error = err;
}
#endif


bool EventPostParams(const std::string& str_request, std::string& reply)
{
    std::string host = "192.168.3.203";
      // Get credentials
    std::string strRPCUserColonPass = "dev:a";   
    int port = 8332;
    
    raii_event_base base = obtain_event_base();

    // Synchronously look up hostname
    raii_evhttp_connection evcon =
        obtain_evhttp_connection_base(base.get(), host, port);
    evhttp_connection_set_timeout(
        evcon.get(),
        DEFAULT_HTTP_CLIENT_TIMEOUT);
    HTTPReply response;
    raii_evhttp_request req =
        obtain_evhttp_request(http_request_done, (void *)&response);
    if (req == nullptr) throw std::runtime_error("create http request failed");
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    evhttp_request_set_error_cb(req.get(), http_error_cb);
#endif


    struct evkeyvalq *output_headers = evhttp_request_get_output_headers(req.get());
    assert(output_headers);
    evhttp_add_header(output_headers, "Host", host.c_str());
    evhttp_add_header(output_headers, "Connection", "close");
    evhttp_add_header(output_headers, "Authorization", (std::string("Basic ") + EncodeBase64((const unsigned char*)strRPCUserColonPass.c_str(), strRPCUserColonPass.size())).c_str());
  // Attach request data
   // std::string strRequest = JSONRPCRequestObj(strMethod, params, 1).dump() + "\n";
    struct evbuffer *output_buffer = evhttp_request_get_output_buffer(req.get());
    assert(output_buffer);
    //evbuffer_add(output_buffer, strRequest.data(), strRequest.size());
    evbuffer_add(output_buffer, str_request.data(), str_request.size());

    // check if we should use a special wallet endpoint
    std::string endpoint = "/";
    int r = evhttp_make_request(evcon.get(), req.get(), EVHTTP_REQ_POST, endpoint.c_str());

    // ownership moved to evcon in above call
    req.release();
    if (r != 0)
    {
        return false;
      //  throw CConnectionFailed("send http request failed");
    }

    event_base_dispatch(base.get());

    reply = response.body;
    std::cout <<reply << std::endl;
    return true;
 /*if (response.status == 0)
    {
        throw CConnectionFailed(strprintf(
            "couldn't connect to server: %s (code %d)\n(make sure server is "
            "running and you are connecting to the correct RPC port)",
            http_errorstring(response.error), response.error));
    }
    else if (response.status == HTTP_UNAUTHORIZED)
    {
        throw std::runtime_error("incorrect rpcuser or rpcpassword (authorization failed)");
    }
    else if (response.status >= 400 && response.status != HTTP_BAD_REQUEST &&
               response.status != HTTP_NOT_FOUND &&
               response.status != HTTP_INTERNAL_SERVER_ERROR)
    {
        throw std::runtime_error(strprintf("server returned HTTP error %d", response.status));
    }
    else if (response.body.empty())
    {
        throw std::runtime_error("no response from server");
    }

    // Parse reply
    json json_reply = json::parse(response.body);
    return json_reply;
    */
}

bool CurlPostParams(const CurlParams &params, std::string &response)
{
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;
    CURLcode res;
    response.clear();
    std::string error_str ;
    //error_str.clear();
    if (curl)
    {
        headers = curl_slist_append(headers, params.content_type.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, params.url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)params.data.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.data.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReplyCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        if(params.need_auth)
        {
            curl_easy_setopt(curl, CURLOPT_USERPWD, params.auth.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
        }
        //curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 0);
        //curl_easy_setopt(curl, CURLOPT_RETURNTRANSFER, 1L);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
       // curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 0L);
        res = curl_easy_perform(curl);
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
	//LOG(INFO) <<  response;
    if (res != CURLE_OK)
    {
        error_str = curl_easy_strerror(res);
        LOG(ERROR)<<"curl error: " <<error_str << std::endl;
        g_node_dump = true;
        return false;
    }
    return true;

}


