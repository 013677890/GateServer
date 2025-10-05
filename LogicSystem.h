#pragma once
#include "global.h"

class HttpConnection;

typedef std::function<void(const std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem> {
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    bool HandleGet(std::string,std::shared_ptr<HttpConnection>);
    void RegGet(std::string, HttpHandler handler);
private:
    LogicSystem();
    std::map<std::string, HttpHandler> _getHandlers;
    std::map<std::string, HttpHandler> _postHandlers;
};