#define BOOST_ASIO_ENABLE_HANDLER_TRACKING
#include <cstdlib>
#include <thread>
#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "../lib/json.hpp"
#include "../lib/Common.hpp"

using boost::asio::ip::tcp;

class Core
{
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

        BidInfo* next_ = nullptr;
    };

    // "Регистрирует" нового пользователя и возвращает его ID.
    std::string RegisterNewUser(const std::string& aUserName)
    {
        size_t newUserId = mUsers.size();
        mUsers[newUserId] = {aUserName, 0, 0};

        return std::to_string(newUserId);
    }

    // Запрос имени клиента по ID
    std::string GetUserName(const std::string& aUserId)
    {
        const auto userIt = mUsers.find(std::stoi(aUserId));
        if (userIt == mUsers.cend())
        {
            return "Error! Unknown User";
        }
        else
        {
            return userIt->second.name_;
        }
    }
    
    std::string GetUserBalance(const std::string& aUserId) {
        const auto userIt = mUsers.find(std::stoi(aUserId));
        if (userIt == mUsers.cend())
        {
            return "Error! Unknown User";
        }
        else
        {
            std::stringstream balance;
            balance << userIt->second.balance_USD_ << " " << userIt->second.balance_RUB_;
            return balance.str();
        }
    }

    std::string CreateNewBid(const std::string& aUserId, const std::string& args) {
        std::stringstream reply;
        reply << "Bid creation from thread " << std::this_thread::get_id() << "\n";
        auto userIt = mUsers.find(std::stoi(aUserId));
        if (userIt == mUsers.cend())
        {
            return "Error! Unknown User";
        }

        std::stringstream args_ss(args);
        uint64_t amount;
        uint64_t price;
        std::string side;
        args_ss >> amount >> price >> side;

        if (side == "sell") {
            while (to_buy_ && (price <= to_buy_->price_) && (amount > 0)) {
                uint64_t min_amount = std::min(amount, to_buy_->amount_);

                userIt->second.balance_RUB_ += min_amount * to_buy_->price_;
                userIt->second.balance_USD_ -= min_amount;
                
                mUsers[to_buy_->uId_].balance_RUB_ -= min_amount * to_buy_->price_;
                mUsers[to_buy_->uId_].balance_USD_ += min_amount;
                
                amount -= min_amount;
                to_buy_->amount_ -= min_amount;
                reply << "Sold " << min_amount << " for " << to_buy_->price_ << "RUB\n";

                if (!to_buy_->amount_) {
                    auto tmp = to_buy_->next_;
                    delete to_buy_;
                    to_buy_ = tmp;
                }
            }
            if (amount <= 0) {
                return reply.str();
            }
            reply << "Bid Created\n";
            
            auto next_node = to_sell_;
            auto now_node = to_sell_;
            while (next_node && (price >= next_node->price_)) {
                now_node = next_node;
                next_node = next_node->next_;
            }
            BidInfo* new_node = new BidInfo {userIt->first, amount, price, next_node};
            if (now_node == next_node) {
                to_sell_ = new_node;
            } else {
                now_node->next_ = new_node;
            }
        } else if (side == "buy") {
            while (to_sell_ && (price >= to_sell_->price_) && (amount > 0)) {
                uint64_t min_amount = std::min(amount, to_sell_->amount_);

                userIt->second.balance_RUB_ -= min_amount * to_sell_->price_;
                userIt->second.balance_USD_ += min_amount;

                mUsers[to_sell_->uId_].balance_RUB_ += min_amount * to_sell_->price_;
                mUsers[to_sell_->uId_].balance_USD_ -= min_amount;
                
                amount -= min_amount;
                to_sell_->amount_ -= min_amount;
                reply << "Bought " << min_amount << " for " << to_sell_->price_ << "RUB\n";

                if (!to_sell_->amount_) {
                    auto tmp = to_sell_->next_;
                    delete to_sell_;
                    to_sell_ = tmp;
                }
            }
            if (amount <= 0) {
                return reply.str();
            }
            reply << "Bid Created\n";
            
            auto next_node = to_buy_;
            auto now_node = to_buy_;
            while (next_node && (price <= next_node->price_)) {
                now_node = next_node;
                next_node = next_node->next_;
            }
            BidInfo* new_node = new BidInfo {userIt->first, amount, price, next_node};
            if (now_node == next_node) {
                to_buy_ = new_node;
            } else {
                now_node->next_ = new_node;
            }
        }
        return reply.str();
    }

    ~Core() {
        while (to_sell_)
        {
            auto tmp = to_sell_->next_;
            delete to_sell_;
            to_sell_ = tmp;
        }
        while (to_buy_)
        {
            auto tmp = to_buy_->next_;
            delete to_buy_;
            to_buy_ = tmp;
        }
    }

private:
    // <UserId, UserName and UserBalance>
    std::unordered_map<size_t, UserInfo> mUsers;
    BidInfo* to_sell_ = nullptr;
    BidInfo* to_buy_ = nullptr;
};

Core& GetCore()
{
    static Core core;
    return core;
}

class session
{
public:
    session(boost::asio::io_service& io_service)
        : socket_(io_service)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            boost::bind(&session::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    // Обработка полученного сообщения.
    void handle_read(const boost::system::error_code& error,
        size_t bytes_transferred)
    {
        if (!error)
        {
            data_[bytes_transferred] = '\0';

            // Парсим json, который пришёл нам в сообщении.
            auto j = nlohmann::json::parse(data_);
            auto reqType = j["ReqType"];

            std::string reply = "Error! Unknown request type";
            if (reqType == Requests::Registration)
            {
                // Это реквест на регистрацию пользователя.
                // Добавляем нового пользователя и возвращаем его ID.
                reply = GetCore().RegisterNewUser(j["Message"]);
            }
            else if (reqType == Requests::Hello)
            {
                // Это реквест на приветствие.
                // Находим имя пользователя по ID и приветствуем его по имени.
                reply = "Hello, " + GetCore().GetUserName(j["UserId"]) + "!\n";
            } 
            else if (reqType == Requests::Balance)
            {
                // Это реквест проверяет баланс
                // Находим имя пользователя по ID и возвращаем его баланс.
                reply = GetCore().GetUserBalance(j["UserId"]);
            }
            else if (reqType == Requests::Bid)
            {
                // Этот реквест созаёт заявку и пополняет счета при возможности
                reply = GetCore().CreateNewBid(j["UserId"], j["Message"]);
            }

            boost::asio::async_write(socket_,
                boost::asio::buffer(reply, reply.size()),
                boost::bind(&session::handle_write, this,
                    boost::asio::placeholders::error));
        }
        else
        {
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                boost::bind(&session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            delete this;
        }
    }

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class server
{
public:
    server(boost::asio::io_service& io_service)
        : io_service_(io_service),
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        std::cout << "Server started! Listen " << port << " port" << std::endl;

        // boost::asio::signal_set sig(io_service, SIGINT, SIGTERM);
        // sig.async_wait(boost::bind(&server::handle_signal, this,
        //         boost::asio::placeholders::error,
        //         boost::asio::placeholders::signal_number));

        session* new_session = new session(io_service_);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
    }

    void handle_accept(session* new_session,
        const boost::system::error_code& error)
    {
        if (!error)
        {
            new_session->start();
            new_session = new session(io_service_);
            acceptor_.async_accept(new_session->socket(),
                boost::bind(&server::handle_accept, this, new_session,
                    boost::asio::placeholders::error));
        }
        else
        {
            delete new_session;
        }
    }

    void handle_signal(const boost::system::error_code& error, int signal) {
        std::cout << "Stopped server on port " << port << ". Because of signal " << signal << std::endl;
        exit(1);
    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        boost::asio::io_service io_service;

        server s(io_service);

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "Unknown exception\n";
    }

    return 0;
}