ENABLE_TESTING()

ADD_EXECUTABLE(Tests test.cpp multy_client_test.cpp logic_test.cpp)
TARGET_LINK_LIBRARIES(Tests PRIVATE ServerLib ClientLib Common gtest gtest_main Threads::Threads ${Boost_LIBRARIES})
TARGET_INCLUDE_DIRECTORIES(Tests PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)

INCLUDE(GoogleTest)
gtest_discover_tests(Tests)