#pragma once
// 全局头文件，放一些常用的系统头文件和第三方库头文件

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <functional>
#include <map>
#include <unordered_map>
#include "Singleton.h"
#include <iostream>
#include <json/json.h> // JSON 库头文件
#include <json/value.h>
#include <json/reader.h>

enum ERROR_CODE {
    ERROR_SUCCESS = 0,
    ERROR_JSON = 1001,
    ERROR_RPCFAILED = 1002,
};