ADD_LIBRARY(Common STATIC 
    Common.cpp
)
TARGET_INCLUDE_DIRECTORIES(Common PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
)
TARGET_LINK_LIBRARIES(Common PUBLIC 
    Threads::Threads 
    ${Boost_LIBRARIES}
)
add_subdirectory(gtest)