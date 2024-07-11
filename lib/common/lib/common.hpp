#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short prod_port = 5555;
static short test_port = 5556;

namespace Requests {
static std::string Balance      = "Bal";
static std::string Bid          = "Bid";
static std::string Registration = "Reg";
static std::string Hello        = "Hel";
static std::string ClearData    = "Cle";
} // namespace Requests

#endif // CLIENSERVERECN_COMMON_HPP
