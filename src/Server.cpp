#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <common.hpp>
#include <cstdlib>
#include <iostream>
#include <json.hpp>
#include <server_lib.hpp>
#include <thread>

using boost::asio::ip::tcp;

int main(int argc, char **argv) {
  try {
    boost::asio::io_service io_service;
    if (!strcmp(argv[1], "test")) {
      server s(io_service, test_port);
      io_service.run();
    } else if (!strcmp(argv[1], "prod")) {
      server s(io_service, test_port);
      io_service.run();
    }
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "Unknown exception\n";
  }

  return 0;
}