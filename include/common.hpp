#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <boost/network/utils/thread_pool.hpp>
#include <boost/fusion/adapted/std_pair.hpp>

typedef unsigned long long int bigserial_t;
typedef long long int fake_bigserial_t;

extern boost::network::utils::thread_pool threadPool;

#endif