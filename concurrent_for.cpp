#include <functional>
#include <iostream>

#include <boost/fiber/all.hpp>

#include "ospcommon/utility/OnScopeExit.h"

#define DEBUG_OUTPUTS  0

const int N_FIBERS  = 3;
const int MAX_VALUE = 10;

thread_local static std::function<void(int)> currentFiberTask;
thread_local static bool newTask = false;
thread_local static bool cancelFibers = false;
thread_local static int numFinishedFibers = 0;

// NOTE: This is for auto-destruction, argueably better as a std::unique_ptr
//       custom deleter...
struct FiberGroup
{
  ~FiberGroup()
  {
    clearFibers();
  }

  void clearFibers()
  {
    if (!activeFibers.empty()) {
#if DEBUG_OUTPUTS
      std::cout << "destroying fibers..." << std::endl;
#endif
      cancelFibers = true;
      newTask = false;

#if DEBUG_OUTPUTS
      std::cout << "joining fibers..." << std::endl;
#endif
      for (auto &f : activeFibers)
        f.join();

      activeFibers.clear();
    }
  }

  std::vector<boost::fibers::fiber> activeFibers;
};

thread_local static FiberGroup fibers;

void fiber_fcn(int whichFiber)
{
  while (true) {
    if (cancelFibers)
      return;

    if (!newTask) {
#if DEBUG_OUTPUTS
      std::cout << "ignoring switch to fiber[" << whichFiber << "]"
                << std::endl;
#endif
      boost::this_fiber::yield();
      continue;
    }

#if DEBUG_OUTPUTS
    std::cout << "starting task on fiber[" << whichFiber << "]" << std::endl;
#endif

    currentFiberTask(whichFiber);

#if DEBUG_OUTPUTS
    std::cout << "finished task on fiber[" << whichFiber << "]" << std::endl;
#endif
    numFinishedFibers++;
    newTask = false;
#if DEBUG_OUTPUTS
    std::cout << "incrementing num finished from fiber[" << whichFiber << "] "
              << numFinishedFibers << std::endl;;
#endif
  }
}

template <typename TASK_T>
inline void concurrent_for(int nFibers, TASK_T&& fcn)
{
  using namespace boost;

  if (::fibers.activeFibers.size() != size_t(nFibers)) {
#if DEBUG_OUTPUTS
    std::cout << "RESIZING FIBERS---------------------" << std::endl;
#endif
    ::fibers.activeFibers.resize(nFibers);

    int whichFiber = 0;
    for (auto &fiber : ::fibers.activeFibers)
      fiber = fibers::fiber(fibers::launch::post, fiber_fcn, whichFiber++);
  } else {
#if DEBUG_OUTPUTS
    std::cout << "IGNORING FIBER RESIZE!!!!!!!!!!" << std::endl;
#endif
  }

  numFinishedFibers = 0;

  currentFiberTask = fcn;
  newTask = true;

#if DEBUG_OUTPUTS
  std::cout << "starting fibers..." << std::endl;
#endif

  while (numFinishedFibers < nFibers) {
#if DEBUG_OUTPUTS
    std::cout << "# finished fibers: " << numFinishedFibers << std::endl;
#endif
    this_fiber::yield();
  }
}

// Client code ////////////////////////////////////////////////////////////////

int main()
{
  ospcommon::utility::OnScopeExit onExit([&](){
#if DEBUG_OUTPUTS
    std::cout << "CLEARING FIBERS------------" << std::endl;
#endif
    ::fibers.clearFibers();
  });

  int value = N_FIBERS;

  concurrent_for(N_FIBERS, [&](int whichFiber) {
    while (value < MAX_VALUE) {
      value++;

      std::cout << "fiber[" << whichFiber << "] "
                << "value now is " << value << std::endl;

      boost::this_fiber::yield();
    }
  });

  std::cout << std::endl;
  std::cout << "starting another concurrent_for()..." << std::endl;
  std::cout << std::endl;

  value = N_FIBERS;

  concurrent_for(N_FIBERS, [&](int whichFiber) {
    while (value < MAX_VALUE) {
      value++;

      std::cout << "fiber[" << whichFiber << "] "
                << "value now is " << value << std::endl;

      boost::this_fiber::yield();
    }
  });

  std::cout << "...done" << std::endl;

  return 0;
}
