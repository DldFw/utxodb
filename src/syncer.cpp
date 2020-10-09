#include "syncer.h"
#include <glog/logging.h>
#include "db_mysql.h"
#include <gmp.h>
#include <gmpxx.h>
#include "task_helpers.h"
std::map<std::string, int> s_map_address_id;
static void SetTimeout(const std::string& name, int second)
{
    struct timeval timeout ;
    timeout.tv_sec = second;
    timeout.tv_usec = 0;
    evtimer_add(Job::map_name_event_[name], &timeout);
}

static void ScanChain(int fd, short kind, void *ctx)
{
    LOG(INFO) << "scan block begin ";

    Syncer::instance().scanBlockChain(); 
    SetTimeout("ScanChain", 60);
}


void Syncer::refreshDB()
{
    LOG(INFO) << "refresh DB begin" ;
    LOG(INFO) << "SQL size: " << vect_sql_.size() ;
    if (vect_sql_.size() > 0)
    {
        g_db_mysql->batchRefreshDB(vect_sql_);
        vect_sql_.clear();
    }   
    LOG(INFO) << "refresh DB end" ;
}

static std::vector<std::string> FormatTransaction(Rpc& rpc, std::string txid)
{
	json json_tx;
	rpc.getRawTransaction(txid, json_tx);
	json json_vins = json_tx["result"]["vin"];
	json json_vouts = json_tx["result"]["vout"];
	std::vector<std::string> vect_sql;
	for (int k = 0; k < json_vins.size(); k++)
	{
		if (json_vins[k].find("txid") == json_vins[k].end())
		{
			continue;
		}
		std::string del_txid = json_vins[k]["txid"].get<std::string>();
		int del_n = json_vins[k]["vout"].get<int>();
		std::string sql = "DELETE FROM unspent WHERE txid ='" +del_txid+ "' AND n='"+ std::to_string(del_n) +"';";
		vect_sql.push_back(sql);
	}

	for (int k = 0; k < json_vouts.size(); k++)
	{
		double value = json_vouts[k]["value"].get<double>();

		if (value  <= 0.00000002 )
		{
			continue;
		}

		int n = json_vouts[k]["n"].get<int>();
		std::string pubkey = json_vouts[k]["scriptPubKey"]["hex"].get<std::string>();
		if (json_vouts[k]["scriptPubKey"].find("addresses") == json_vouts[k]["scriptPubKey"].end())
		{
			continue;
		}

		std::string address = json_vouts[k]["scriptPubKey"]["addresses"][0].get<std::string>();
		std::string sql  = "INSERT INTO `unspent` VALUES ('" + txid  + "', '"+std::to_string(n)+"', '"+ std::to_string(value) +"', '"+ address +"', '" + pubkey +"');";
		vect_sql.push_back(sql);
	}
	return vect_sql;
}
static CThreadPool<CQueueAdaptor> pool { "TxPool", 12 };
void Syncer::scanBlockChain()
{
	uint64_t pre_height = 0;
	std::string sql_height = "select height from block order by height desc limit 1;";
	std::map<int,DBMysql::DataType> map_col_type;
	map_col_type[0] = DBMysql::INT;

	json json_data;
	g_db_mysql->getData(sql_height, map_col_type, json_data);
	if (json_data.size() > 0)
	{
		pre_height = json_data[0][0].get<uint64_t>();
	}

	try 
	{	
		uint64_t cur_height = pre_height;
		rpc_.getBlockCount(cur_height);
		json json_block;
		end_ = cur_height;

		begin_ = pre_height;
		for (int i = pre_height + 1; i <= cur_height; i++)
		{
			json_block.clear();

			rpc_.getBlock(i, json_block);
			if (g_node_dump)
			{
				g_node_dump = false;
				break;  
			}
			json json_txs = json_block["result"]["tx"];

	//		CThreadPool<CQueueAdaptor> pool { "TxPool", 12 };
			std::vector<std::future<std::vector<std::string>> > results {};
			for (int j = 1; j < json_txs.size(); j++)
			{
				std::string txid = json_txs[j].get<std::string>();
				results.push_back(make_task(pool,FormatTransaction, rpc_, txid));
			}

			LOG(INFO) << "TASK END";
			for(auto& res: results)
			{
				std::vector<std::string> vect_sql = res.get();
				for(int k =0; k < vect_sql.size(); k++)
					vect_sql_.push_back(vect_sql[k]);
			}
			//LOG(INFO) << "block height: " << i;
			//if(i % 5 == 0 || i == cur_height)
			{

				LOG(INFO) << "block height: " << i;
				std::string sql = "INSERT INTO `block` VALUES ('" + std::to_string(i) + "');";
				vect_sql_.push_back(sql);
				refreshDB();
				begin_ = i;	
			}
		}
	}
	catch(...)
	{
		LOG(ERROR) << "from: "  << begin_ << " to: " << end_;
		if (end_ - begin_ > 1)
		{
			scanBlockChain();
		}
	}

}


Syncer Syncer::single_;
void Syncer::registerTask(map_event_t& name_events, map_job_t& name_tasks)
{
    REFLEX_TASK(ScanChain);
}


