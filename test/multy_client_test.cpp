#include <gtest/gtest.h>
#include <boost/thread/thread.hpp>
#include <client_lib.hpp>
#include <common.hpp>

void ThreadTask(size_t ind) {
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), "127.0.0.1", std::to_string(test_port));
    tcp::resolver::iterator iterator = resolver.resolve(query);

    tcp::socket s(io_service);
    ASSERT_NO_THROW(s.connect(*iterator));
    
    ASSERT_NO_THROW(SendMessage(s, "0", Requests::Registration, "client_" + std::to_string(ind)));
    std::string id = ReadMessage(s);

    ASSERT_NO_THROW(SendMessage(s, id, Requests::Hello, "0"));
    ReadMessage(s);
    io_service.stop();
}

TEST(ServerLoadTest, MultipleClients)
{
	boost::thread_group threadpool;
    for (size_t i = 0; i < 3; ++i) {
        threadpool.create_thread(boost::bind(&ThreadTask, i));
    }
    threadpool.join_all();
}