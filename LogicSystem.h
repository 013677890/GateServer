#pragma once
#include "global.h"

class HttpConnection;

typedef std::function<void(const std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem> {
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    bool HandleGet(std::string path,const std::shared_ptr<HttpConnection> conn);
    bool HandlePost(std::string path,const std::shared_ptr<HttpConnection> conn);
    void RegGet(std::string, HttpHandler handler);
    void RegPost(std::string, HttpHandler handler);
    bool Init();
private:
    LogicSystem();
    // 在这里声明所有路由处理函数 ---
    void Handle_GetTest(const std::shared_ptr<HttpConnection> conn);
    void Handle_GetVarifyCode(const std::shared_ptr<HttpConnection> conn);
    std::map<std::string, HttpHandler> _getHandlers;
    std::map<std::string, HttpHandler> _postHandlers;
    bool _isInited = false;
};