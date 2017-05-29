#include <iostream>

#include <boost/fiber/all.hpp>

using channel_t = boost::fibers::unbuffered_channel<unsigned int>;

void foo(channel_t & chan)
{
  chan.push(1);
  chan.push(1);
  chan.push(2);
  chan.push(3);
  chan.push(5);
  chan.push(8);
  chan.push(12);
  chan.close();
}

void bar(channel_t & chan)
{
  for (auto value : chan)
    std::cout << value << " ";

  std::cout << std::endl;
}

int main()
{
	try {
    channel_t chan;

    boost::fibers::fiber f1(&foo, std::ref(chan));
    boost::fibers::fiber f2(&bar, std::ref(chan));

    f1.join();
    f2.join();

		std::cout << "done." << std::endl;

		return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    std::cerr << "exception: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "unhandled exception" << std::endl;
  }

	return EXIT_FAILURE;
}
