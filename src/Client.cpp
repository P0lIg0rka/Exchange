#include <boost/asio.hpp>
#include <client_lib.hpp>
#include <common.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main() {
  try {
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), "127.0.0.1", std::to_string(prod_port));
    tcp::resolver::iterator iterator = resolver.resolve(query);

    tcp::socket s(io_service);
    s.connect(*iterator);

    // Мы предполагаем, что для идентификации пользователя будет использоваться
    // ID. Тут мы "регистрируем" пользователя - отправляем на сервер имя, а
    // сервер возвращает нам ID. Этот ID далее используется при отправке
    // запросов.
    std::string my_id = ProcessRegistration(s);

    while (true) {
      // Тут реализовано "бесконечное" меню.
      std::cout << "Menu:\n"
                   "1) Hello Request\n"
                   "2) Balance\n"
                   "3) Create a Bid\n"
                   "4) Exit\n"
                << std::endl;

      int32_t menu_option_num;
      std::string param;
      std::cin >> param;
      try {
        menu_option_num = std::stoi(param);
      } catch (std::invalid_argument const &) {
        std::cout << "Select a number!\n";
        continue;
      }
      switch (menu_option_num) {
      case 1: {
        SendMessage(s, my_id, Requests::Hello, "");
        std::cout << ReadMessage(s);
        break;
      }
      case 2: {
        SendMessage(s, my_id, Requests::Balance, "");
        std::stringstream balance(ReadMessage(s));
        int64_t usd, rub;
        balance >> usd >> rub;
        std::cout << "RUB: " << rub << std::endl << "USD: " << usd << std::endl;
        break;
      }
      case 3: {
        std::stringstream args;
        std::string side;
        size_t pos;

        std::cout << "Enter amount of $$$\n";
        std::cin >> param;
        try {
          std::stoull(param, &pos);
          args << param.substr(0, pos) << " ";
        } catch (std::invalid_argument const &) {
          std::cout << "Not correct number!\n";
          break;
        }

        std::cout << "Enter price of $$$\n";
        std::cin >> param;
        try {
          std::stoull(param, &pos);
          args << param.substr(0, pos) << " ";
        } catch (std::invalid_argument const &) {
          std::cout << "Not correct number!\n";
          break;
        }

        std::cout << "Write sell or buy\n";
        std::cin >> side;
        if ((side != "sell") && (side != "buy")) {
          std::cout << "Not correct side!\n";
          break;
        }
        args << side;
        SendMessage(s, my_id, Requests::Bid, args.str());
        std::cout << ReadMessage(s);
        break;
      }
      case 4: {
        exit(0);
        break;
      }
      default: {
        std::cout << "Unknown menu option\n" << std::endl;
      }
      }
    }
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}