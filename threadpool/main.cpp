#include "task_helpers.h"
#include <string>
#include <iostream>


std::string Function(int data, std::string ceshi)
{
	return std::to_string(data) +  ceshi;
}



int main(int argc, char* argv[])
{
    CThreadPool<CQueueAdaptor> pool { "TestPool", 8 };
    // Submit some tasks to the queue
    std::vector<std::future<std::string>> results {};
	for (int i =0; i < 1000000; i++)
	{
    	results.push_back(make_task(pool, Function, i, std::to_string(i)));
	}
   /* results.push_back(make_task(pool, Function, 1, "1"));
    results.push_back(make_task(pool, Function, 2, "2"));
    results.push_back(make_task(pool, Function, 3, "3"));
    results.push_back(make_task(pool, Function, 4, "4"));
    results.push_back(make_task(pool, Function, 5, "4"));
*/
    // Wait for all tasks to complete
    for(auto& res : results)
    {
		std::cout << "ret: " <<  res.get() << std::endl;
    }

	return 0;
}
