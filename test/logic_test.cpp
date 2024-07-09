#include <gtest/gtest.h>
#include <common.hpp>
#include <thread>
#include <server_lib.hpp>
#include <client_lib.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

typedef std::vector<std::pair<size_t, std::string>> Arguments;
typedef std::tuple<int64_t, int64_t, Arguments> Params;

class ServerLogicTest : public ::testing::TestWithParam<Params> {
protected:
  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}

  void SetUp() override {
    test_server_up = true;
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), "127.0.0.1", std::to_string(test_port));
    tcp::resolver::iterator iterator = resolver.resolve(query);

    s_ = new tcp::socket(io_service);
    try {
      s_->connect(*iterator);

      uIds_.clear();
      uIds_.resize(2);
      SendMessage(*s_, "0", Requests::Registration, "first");
      uIds_[0] = ReadMessage(*s_);
      SendMessage(*s_, "0", Requests::Registration, "second");
      uIds_[1] = ReadMessage(*s_);
    } catch (...) {
      test_server_up = false;
    }
  }

  void TearDown() override {
    if (test_server_up) {
      SendMessage(*s_, "0", Requests::ClearData, "0");
      ReadMessage(*s_);
    }
    delete s_;
  }
public:
  std::vector<std::string> uIds_;
  static tcp::socket* s_;
  bool test_server_up;
};

tcp::socket* ServerLogicTest::s_ = nullptr;



TEST_P(ServerLogicTest, BidsCreation) {
  ASSERT_TRUE(test_server_up);
  for (const auto& arg: std::get<2>(GetParam())) {
    SendMessage(*s_, uIds_[arg.first], Requests::Bid, arg.second);
    ReadMessage(*s_);
  }

  std::pair<int64_t, int64_t> first_balance;
  std::pair<int64_t, int64_t> second_balance;

  SendMessage(*s_, uIds_[0], Requests::Balance, "");
  std::stringstream bal1(ReadMessage(*s_));
  bal1 >> first_balance.first >> first_balance.second;

  SendMessage(*s_, uIds_[1], Requests::Balance, "");
  std::stringstream bal2(ReadMessage(*s_));
  bal2 >> second_balance.first >> second_balance.second;

  ASSERT_EQ(first_balance.first, -second_balance.first);
  ASSERT_EQ(first_balance.second, -second_balance.second);
  ASSERT_EQ(first_balance.second, std::get<0>(GetParam()));
  ASSERT_EQ(second_balance.first, std::get<1>(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
  BidCreationCorrectness,
  ServerLogicTest,
  ::testing::Values(
    std::make_tuple<int64_t, int64_t, Arguments>(
      200,
      15,
      {
        {0, "10 10 sell"},
        {0, "10 20 sell"},
        {1, "15 25 buy"}
      }
    ),
    std::make_tuple<int64_t, int64_t, Arguments>(
      0,
      0,
      {
        {0, "10 10 sell"},
        {0, "10 20 sell"},
        {0, "10 15 sell"},
        {1, "15 9 buy"},
        {1, "15 8 buy"},
        {1, "15 6 buy"}
      }
    ),
    std::make_tuple<int64_t, int64_t, Arguments>(
      200,
      15,
      {                    //USD RUB
        {0, "10 10 sell"}, // 0   0
        {1, "5 15 buy"},   // 5  -50
        {0, "10 20 sell"}, // 5  -50
        {1, "10 25 buy"},  //15 -200
        {1, "15 15 buy"},
        {1, "15 10 buy"}
      }
    )
  )
);