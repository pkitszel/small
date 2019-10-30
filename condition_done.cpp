 /* simplifying wrapper for std::condition_variable, c++11

make flags:
	CXXFLAGS+='-Wall -Wextra' LDFLAGS+=-pthread
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
/*
 * simplifying wrapper for std::condition_variable
 *
 * usage:
    condition_done cond;

    // in waiting thread:
    auto mtx = cond.wait(); // to wait until done, and unblock all waiters synchronously
    cond.wait(); // to wait until done without care about other waiters

    // in unblocking thread:
    cond.notif_done();
*/
class condition_done {
	std::mutex mtx;
	std::condition_variable cv;
	bool done = false;
public:
	std::unique_lock<std::mutex> wait() {
		std::unique_lock<std::mutex> lck(mtx);
		cv.wait(lck, [this](){return done;});
		return lck;
	}
	void notif_done() {
		std::unique_lock<std::mutex> lck(mtx);
		done = true;
		cv.notify_all();
	}
};

// modified example from http://www.cplusplus.com/reference/condition_variable/condition_variable/notify_all/

condition_done cd;

void print_id (int id) {
  auto x = cd.wait();
  // ...
  std::cout << "thread " << id << '\n';
}

void go() {
  cd.notif_done();
}

int main ()
{
  std::thread threads[10];
  // spawn 10 threads:
  for (int i=0; i<10; ++i)
    threads[i] = std::thread(print_id,i);

  std::cout << "10 threads ready to race...\n";
  go();                       // go!

  for (auto& th : threads) th.join();

  return 0;
}
