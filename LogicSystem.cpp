#include "LogicSystem.h"
#include "HttpConnection.h"//不在头文件中包含，避免循环依赖

LogicSystem::~LogicSystem() {
    
}

LogicSystem::LogicSystem() {
    spdlog::info("LogicSystem destroyed.");
    RegGet("/get_test", [](const std::shared_ptr<HttpConnection> conn) {
        boost::beast::ostream(conn->_res.body())
            << "This is a test response from /get_test endpoint.\r\n";
    });
    spdlog::info("Registered /get_test handler.");
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> conn) {
    auto it = _getHandlers.find(path);
    if (it != _getHandlers.end()) {
        // 找到处理函数，调用它
        it->second(conn);
        return true;
    }
    return false; // 未找到处理函数
}

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
    _getHandlers.insert(std::make_pair(url, handler));
}
