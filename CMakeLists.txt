cmake_minimum_required(VERSION 3.10)

project(SearchEngine VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# 查找线程库
find_package(Threads REQUIRED)

# 添加包含路径
include_directories(.)
include_directories(/usr/include/nlohmann)
include_directories(/usr/local/include)

# 定义源文件集合
set(COMMON_SOURCES
    src/keywords/common.cc
    src/keywords/english_tokenizer.cc
    src/keywords/chinese_tokenizer.cc
    src/keywords/unified_tokenizer.cc
)

set(DATA_READER_SOURCES
    src/data_reader/data_reader.cc
)

# Reactor模块
set(NETWORK_SOURCES
    src/network/thread_pool.cc
    src/network/task_queue.cc
    src/network/socket.cc
    src/network/socket_io.cc
    src/network/inet_address.cc
    src/network/tcp_connection.cc
    src/network/acceptor.cc
    src/network/event_loop.cc
    src/network/tcp_server.cc
    src/network/tlv_protocol.cc
)

set(SERVER_SOURCES
    src/server/search_engine_server.cc
)

set(WEB_SEARCH_SOURCES
    src/web_search/web_search_engine.cc
)

set(RECOMMENDATION_SOURCES
    src/recommendation/keyword_recommender.cc
)

# 构建搜索引擎服务器
add_executable(search_server
    src/server/search_server.cc
    ${SERVER_SOURCES}
    ${NETWORK_SOURCES}
    ${DATA_READER_SOURCES}
    ${WEB_SEARCH_SOURCES}
    ${RECOMMENDATION_SOURCES}
    ${COMMON_SOURCES}
)

# 构建搜索引擎客户端
add_executable(search_client
    src/client/search_client.cc
    src/network/tlv_protocol.cc
)

# 构建词典生成工具
add_executable(build_dictionaries
    tools/build_dictionaries.cc
    ${COMMON_SOURCES}
)

# 构建网页库生成工具
add_executable(build_webpages
    tools/build_webpages.cc
    src/webpages/page_processor.cc
    ${COMMON_SOURCES}
)

# 构建配置测试工具
add_executable(config_test
    tools/config_test.cc
    src/config/config.cc
)

# 链接线程库和其他依赖库
target_link_libraries(search_server Threads::Threads)
target_link_libraries(search_client Threads::Threads)
target_link_libraries(build_dictionaries Threads::Threads)
target_link_libraries(build_webpages Threads::Threads tinyxml2)
target_link_libraries(config_test Threads::Threads)

# 设置输出目录为项目根目录
set_target_properties(search_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

set_target_properties(search_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

set_target_properties(build_dictionaries PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

set_target_properties(build_webpages PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

set_target_properties(config_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 打印构建信息
message(STATUS "=== 搜索引擎项目构建配置 ===")
message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "编译器: ${CMAKE_CXX_COMPILER}")
message(STATUS "编译标志: ${CMAKE_CXX_FLAGS}")
message(STATUS "=============================")
