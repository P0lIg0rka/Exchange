#include <boost/asio.hpp>
#include <iostream>

#include <common.hpp>
#include <json.hpp>
#include <client_lib.hpp>

using boost::asio::ip::tcp;

// Отправка сообщения на сервер по шаблону.
void SendMessage(tcp::socket &aSocket, const std::string &aId,
                 const std::string &aRequestType, const std::string &aMessage) {
  nlohmann::json req;
  req["UserId"] = aId;
  req["ReqType"] = aRequestType;
  req["Message"] = aMessage;

  std::string request = req.dump();
  boost::asio::write(aSocket, boost::asio::buffer(request, request.size()));
}

// Возвращает строку с ответом сервера на последний запрос.
std::string ReadMessage(tcp::socket &aSocket) {
  boost::asio::streambuf b;
  boost::asio::read_until(aSocket, b, "\0");
  std::istream is(&b);
  std::string line(std::istreambuf_iterator<char>(is), {});
  return line;
}

// "Создаём" пользователя, получаем его ID.
std::string ProcessRegistration(tcp::socket &aSocket) {
  std::string name;
  std::cout << "Hello! Enter your name: ";
  std::cin >> name;

  // Для регистрации Id не нужен, заполним его нулём
  SendMessage(aSocket, "0", Requests::Registration, name);
  return ReadMessage(aSocket);
}