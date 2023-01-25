#include "common.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>
#include <boost/network/tags.hpp>
#include <boost/network/utils/thread_pool.hpp>
#include <boost/shared_ptr.hpp>

#define THREADS_COUNT 10

// io service for thread pool
boost::shared_ptr<boost::asio::io_service> iOService(
        new boost::asio::io_service(THREADS_COUNT));
// lets prevent io_servvice shutdown when its workers quit
//boost::shared_ptr<boost::asio::io_service::work> iOServiceWork(
//        new boost::asio::io_service::work(boost::ref(*iOService)));
// thread group for thread pool
boost::shared_ptr<boost::thread_group> threadGroup(
        new boost::thread_group());

/* create thread pool for THREADS_COUNT threads using the io_service and thread_group
   we recently allocated */
boost::shared_ptr<boost::network::utils::thread_pool> threadPool(
    new boost::network::utils::thread_pool(THREADS_COUNT, iOService, threadGroup));
