#include "common.hpp"

#include <boost/network/tags.hpp>
#include <boost/network/utils/thread_pool.hpp>

#define THREADS_COUNT 10

// create thread pool for THREADS_COUNT threads
boost::network::utils::thread_pool threadPool(THREADS_COUNT);

