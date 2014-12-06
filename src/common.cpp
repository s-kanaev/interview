#include "common.hpp"

#include <boost/network/tags.hpp>
#include <boost/network/utils/thread_pool.hpp>
#include <boost/shared_ptr.hpp>

#define THREADS_COUNT 10

// create thread pool for THREADS_COUNT threads
boost::shared_ptr<boost::network::utils::thread_pool> threadPool(
    new boost::network::utils::thread_pool(THREADS_COUNT));
