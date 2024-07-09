#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

void SendMessage(tcp::socket &aSocket, const std::string &aId,
                 const std::string &aRequestType, const std::string &aMessage);

std::string ReadMessage(tcp::socket &aSocket);

std::string ProcessRegistration(tcp::socket &aSocket);
#endif
