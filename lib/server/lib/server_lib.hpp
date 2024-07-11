#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <cstdlib>

using boost::asio::ip::tcp;

class Core {
public:
  struct UserInfo {
    std::string name_;
    int64_t balance_USD_;
    int64_t balance_RUB_;
  };

  struct BidInfo {
    size_t uId_;
    uint64_t amount_;
    uint64_t price_;

    BidInfo *next_ = nullptr;
  };

  // "Регистрирует" нового пользователя и возвращает его ID.
  std::string RegisterNewUser(const std::string &aUserName);

  // Запрос имени клиента по ID
  std::string GetUserName(const std::string &aUserId);

  std::string GetUserBalance(const std::string &aUserId);

  std::string CreateNewBid(const std::string &aUserId, const std::string &args);

  void Reset();

  ~Core();

private:
  // <UserId, UserName and UserBalance>
  std::unordered_map<size_t, UserInfo> mUsers;
  BidInfo *to_sell_ = nullptr;
  BidInfo *to_buy_  = nullptr;
};

Core &GetCore();

class session {
public:
  session(boost::asio::io_service &io_service);

  tcp::socket &socket();

  void start();

  // Обработка полученного сообщения.
  void handle_read(const boost::system::error_code &error, size_t bytes_transferred);

  void handle_write(const boost::system::error_code &error);

private:
  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server {
public:
  server(boost::asio::io_service &io_service, short port);

  void handle_accept(session *new_session, const boost::system::error_code &error);

private:
  boost::asio::io_service &io_service_;
  tcp::acceptor acceptor_;
};
#endif