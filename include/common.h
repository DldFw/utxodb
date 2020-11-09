#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
#include <vector>
#include  <map>
#include "json.hpp"
using json = nlohmann::json;

struct Node
{
    std::string url;

    std::string host;
    int port;

    std::string auth;
    std::string content_type;
    bool need_auth;
    Node()
    {
        need_auth = true;
        content_type = "content-type:application/json";
    }
    
};


enum HTTPStatusCode
{
    HTTP_NICE                    = 200,
    HTTP_BAD_REQUEST           = 400,
    HTTP_UNAUTHORIZED          = 401,
    HTTP_FORBIDDEN             = 403,
    HTTP_NOT_FOUND             = 404,
    HTTP_BAD_METHOD            = 405,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_SERVICE_UNAVAILABLE   = 503,
};


struct HttpParams
{
    Node* node;
    json json_post;
};

bool EventPost(const HttpParams& params, std::string& reply);
bool CurlPost(const HttpParams& params, std::string& response);

extern bool g_node_dump;
#endif
