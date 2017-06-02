#include <functional>
#include <iostream>

#include <boost/fiber/all.hpp>

const int N_FIBERS  = 3;
const int MAX_VALUE = 10;

// NOTE: This is for auto-destruction, argueably better as a std::unique_ptr
//       custom deleter...
struct FiberGroup
{
  ~FiberGroup()
  {
    for (auto &f : activeFibers)
      f.join();
  }

  std::vector<boost::fibers::fiber> activeFibers;
};

thread_local static std::function<void(int)> currentFiberTask;
thread_local static FiberGroup fibers;
thread_local static bool newTask = false;
thread_local static int numFinishedFibers = 0;

void fiber_fcn(int whichFiber)
{
  if (!newTask)
    boost::this_fiber::yield();

  std::cout << "starting task on fiber [" << whichFiber << "]" << std::endl;

  currentFiberTask(whichFiber);

  std::cout << "finished task on fiber [" << whichFiber << "]" << std::endl;

  newTask = false;
  std::cout << "incrementing num finished from fiber [" << whichFiber << "] "
            << numFinishedFibers++ << std::endl;;
}

template <typename TASK_T>
inline void concurrent_for(int nFibers, TASK_T&& fcn)
{
  fibers.activeFibers.resize(nFibers);

  int whichFiber = 0;
  for (auto &fiber : fibers.activeFibers)
    fiber = boost::fibers::fiber(fiber_fcn, whichFiber++);

  numFinishedFibers = 0;

  currentFiberTask = fcn;
  newTask = true;

  boost::this_fiber::yield();
}

int main()
{
  int value = N_FIBERS;

  concurrent_for(N_FIBERS, [&](int whichFiber) {
    while (value < MAX_VALUE) {
#if 0
      value--;
#else
      value++;
#endif

      std::cout << "fiber[" << whichFiber << "] "
                << "value now is " << value << std::endl;

#if 0
      concurrent_for(2, [&](int whichSubFiber) {
        value++;
        std::cout << "...subfiber[" << whichSubFiber << "] "
                  << "value now is " << value << std::endl;
      });
#endif

      boost::this_fiber::yield();
    }
  });

  while (numFinishedFibers < N_FIBERS) {
    std::cout << "# finished fibers: " << numFinishedFibers << std::endl;
    boost::this_fiber::yield();
  }

  std::cout << "...done" << std::endl;

  return 0;
}
