#include "rpc.h"
#include <glog/logging.h>
#include <iostream>
#include <chrono>
#include <fstream>
bool Rpc::structRpc(const std::string& method, const json& json_params, json& json_post)
{
    json_post["jsonrpc"] = "2.0";
    json_post["id"] = "curltest";
    json_post["method"] = method;
    json_post["params"] = json_params;

    return true;
}

bool Rpc::getBlockCount(uint64_t& height)
{
    json json_post;
    json json_params = json::array();
    json_post["params"] = json_params;

    structRpc("getblockcount", json_params, json_post);
    json json_response;	
    if ( !rpcNode(json_post, json_response) )
    {
        return false;
    }
   	height = json_response["result"].get<uint64_t>();
    return true;
}

bool Rpc::getBlock(const uint64_t& height, json& json_block)
{
    json json_post;
    json json_params;

    json_params.push_back(height);
    json_post["params"] = json_params;
    structRpc("getblockhash", json_params, json_post);
    //    json json_response;	
    if ( !rpcNode(json_post, json_block) )
    {
        return false;
    }
	std::string block_hash = json_block["result"].get<std::string>();
	
	json_params.clear();
	json_post.clear();
	json_params.push_back(block_hash);
	json_post["params"] = json_params;
	json_block.clear();
    structRpc("getblock", json_params, json_post);
    //    json json_response;	
    if ( !rpcNode(json_post, json_block) )
    {
		LOG(ERROR) << json_block.dump(4);
        return false;
    }
	
    return true;
}

bool Rpc::getRawMempool(json& json_rawmempool)
{
    json json_post;
    json json_params = json::array();

    json_post["params"] = json_params;
    structRpc("getrawmempool", json_params, json_post);
    //    json json_response;	
    json json_block;
    if ( !rpcNode(json_post, json_block) )
    {
        return false;
    }
	json_rawmempool = json_block["result"];
	
    return true;
}
        
bool Rpc::getRawTransaction(const std::string& txid, json& json_tx)
{
    json json_post;
    json json_params;
    json_params.push_back(txid);
    json_params.push_back(true);
    json_post["params"] = json_params;

    structRpc("getrawtransaction", json_params, json_post);
    if ( !rpcNode(json_post, json_tx) )
    {
		LOG(ERROR) << json_tx.dump(4);
        return false;
    }

    return true;
}

bool Rpc::rpcNode(const json &json_post, json& json_response)
{
    HttpParams http_params;
    http_params.node = node_;
    http_params.json_post = json_post;
   
    std::string response;
    bool ret = EventPost(http_params, response);
    if(!ret)
    {
        LOG(ERROR) << "CORE ERROR: " << json_post.dump(4);
        return false;
    }

    json_response = json::parse(response);
    if (!json_response["error"].is_null())
    {
        LOG(ERROR) << "node return error";
        LOG(ERROR) << json_post.dump(4);
        LOG(ERROR) << response;
        return false;
    }
    return ret;
}







