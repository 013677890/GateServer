#pragma once

#include "global.h"

class LogicSystem;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
    friend class LogicSystem;
    HttpConnection(boost::asio::ip::tcp::socket socket);
    void Start();
private:
    // 检测超时函数
    void CheckTimeout();
    // 应答函数
    void WriteResponse();
    // 解析请求头
    void HandleRequest();
    // 解析url参数
    void ParseUrlParams();

    boost::asio::ip::tcp::socket _socket;
    boost::beast::flat_buffer _buffer{8192}; // 8KB 缓冲区
    boost::beast::http::request<boost::beast::http::dynamic_body> _req;
    boost::beast::http::response<boost::beast::http::dynamic_body> _res;
    boost::asio::steady_timer _timer;   // 超时定时器
    std::unordered_map<std::string, std::string> _url_params; // 存储url参数
};