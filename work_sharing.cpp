//          Copyright Nat Goodspeed + Oliver Kowalke 2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#include <boost/assert.hpp>

#include <boost/fiber/all.hpp>

#include "barrier.hpp"

static std::size_t fiber_count{0};
static std::mutex mtx_count{};
static boost::fibers::condition_variable_any cnd_count{};

using lock_t = std::unique_lock<std::mutex>;

/*****************************************************************************
*   example fiber function
*****************************************************************************/
//[fiber_fn_ws
void whatevah(char me)
{
  try {
    std::thread::id my_thread = std::this_thread::get_id();

    {
      std::ostringstream buffer;
      buffer << "fiber " << me << " started on thread " << my_thread << '\n';
      std::cout << buffer.str() << std::flush;
    }

    for (unsigned i = 0; i < 10; ++i) {
      boost::this_fiber::yield();
      std::thread::id new_thread = std::this_thread::get_id();

      /*< test if fiber was migrated to another thread >*/
      if (new_thread != my_thread) {
        my_thread = new_thread;
        std::ostringstream buffer;
        buffer << "fiber " << me << " switched to thread " << my_thread << '\n';
        std::cout << buffer.str() << std::flush;
      }
    }
  } catch ( ... ) {
  }

  lock_t lk( mtx_count);

  /*< Decrement fiber counter for each completed fiber. >*/
  if (0 == --fiber_count) {
    lk.unlock();
    cnd_count.notify_all(); /*< Notify all fibers waiting on `cnd_count`. >*/
  }
}
//]

/*****************************************************************************
*   example thread function
*****************************************************************************/
//[thread_fn_ws
void thread(barrier * b)
{
  std::ostringstream buffer;
  buffer << "thread started " << std::this_thread::get_id() << std::endl;
  std::cout << buffer.str() << std::flush;

  /*< Install the scheduling algorithm `boost::fibers::algo::shared_work` in
   *  order to join the work sharing.
  >*/
  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();

  /*< sync with other threads: allow them to start processing >*/
  b->wait();
  lock_t lk( mtx_count);

  /*< Suspend main fiber and resume worker fibers in the meanwhile.
   *  Main fiber gets resumed (e.g returns from
   *  `condition_variable_any::wait()`) if all worker fibers are complete.
  >*/
  cnd_count.wait( lk, [](){ return 0 == fiber_count; } );
  BOOST_ASSERT( 0 == fiber_count);
}
//]

int main(int argc, char *argv[])
{
  std::cout << "main thread started " << std::this_thread::get_id() << std::endl;

  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();
  /*< Install the scheduling algorithm `boost::fibers::algo::shared_work` in the
   *  main thread too, so each new fiber gets launched into the shared pool.
  >*/

  for (char c : std::string("abcdefghijklmnopqrstuvwxyz")) {
    /*< Launch a number of worker fibers; each worker fiber picks up a character
     *  that is passed as parameter to fiber-function `whatevah`.
     *  Each worker fiber gets detached.
    >*/
    boost::fibers::fiber([=](){ whatevah(c); }).detach();
    ++fiber_count; /*< Increment fiber counter for each new fiber. >*/
  }

  barrier b(4);

  std::thread threads[] = {
    /*< Launch a couple of threads that join the work sharing. >*/
    std::thread(thread, & b),
    std::thread(thread, & b),
    std::thread(thread, & b)
  };

  b.wait(); /*< sync with other threads: allow them to start processing >*/

  {
    lock_t lk( mtx_count);
    cnd_count.wait(lk, [](){ return 0 == fiber_count; });
    /*< Suspend main fiber and resume worker fibers in the meanwhile.
        Main fiber gets resumed (e.g returns from `condition_variable_any::wait()`)
        if all worker fibers are complete.
    >*/
  } /*<
      Releasing lock of mtx_count is required before joining the threads,
      otherwise the other threads would be blocked inside
      condition_variable::wait() and would never return (deadlock).
   >*/
  BOOST_ASSERT(fiber_count == 0);
  for (std::thread & t : threads)
    t.join();

  std::cout << "done." << std::endl;
  return EXIT_SUCCESS;
}