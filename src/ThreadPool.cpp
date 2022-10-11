// ThreadPool.cpp: 定义应用程序的入口点。
//

#include "ThreadPool.h"

using namespace std;

int main()
{
	ThreadPool pool(4);
	pool.submit([] {
		std::cout << "threadpool" << std::endl;
		});
	std::this_thread::sleep_for(std::chrono::seconds(1));
	pool.ShutDown();
	return 0;
}
