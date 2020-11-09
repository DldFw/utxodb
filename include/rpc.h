#ifndef RPC_H_
#define RPC_H_
#include "common.h"
#include <iostream>

class Rpc
{
public:
    Rpc()
    {
        node_ = nullptr;
    }

    ~Rpc()
    {
        node_= nullptr;
    }

public:
   
    bool getBlockCount(uint64_t& height);

	bool getBlock(const uint64_t& height, json& json_block);

	bool getRawTransaction(const std::string& txid, json& json_tx);

    bool getRawMempool(json& json_rawmempool);
public:
    bool setNode(Node* node)
    {
        node_ = node;
        return true;
    }    

    bool rpcNode(const json& json_post, json& json_response);

    bool structRpc(const std::string& method, const json& json_params, json& json_post);	
protected:
    Node* node_;

};
#endif // RPC_H

