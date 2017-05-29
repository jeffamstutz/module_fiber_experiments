#include <iostream>

#include <boost/fiber/all.hpp>

const int N_FIBERS  = 3;
const int MAX_VALUE = 10;

template <typename TASK_T>
inline void concurrent_for(int nFibers, TASK_T&& fcn)
{
  std::vector<boost::fibers::fiber> fibers;

  for (int i = 0; i < nFibers; ++i)
    fibers.emplace_back(fcn);

  for (auto &f : fibers)
    f.join();
}

int main()
{
  int value = 0;

  concurrent_for(N_FIBERS, [&]() {
    while (value < MAX_VALUE) {
      value++;
      std::cout << "fiber[" << boost::this_fiber::get_id() << "] "
                << "value now is " << value << std::endl;
      boost::this_fiber::yield();
    }
  });

  std::cout << "...done" << std::endl;

  return 0;
}
