#include "HttpConnection.h"
#include "LogicSystem.h"
HttpConnection::HttpConnection(boost::asio::ip::tcp::socket socket)
    : _socket(std::move(socket))
    , _timer(_socket.get_executor(), std::chrono::seconds(60)) // 60 秒超时
{
}

void HttpConnection::Start()
{
    // 启动超时检测
    CheckTimeout();
    // 异步读取请求
    auto self = shared_from_this();
    boost::beast::http::async_read(
        _socket,
        _buffer,
        _req,
        [self](boost::system::error_code ec, std::size_t bytes_transferred)
        {
            try {
                if (ec) {
                    spdlog::error("Read error: {}", ec.message());
                    return;
                }
                boost::ignore_unused(bytes_transferred);//http服务器可以忽略已发送的字节数（不需要粘包处理
                self->HandleRequest();
                //启动超时检测
                self->CheckTimeout();
            }
            catch (const std::exception& ex) {
                // 处理异常，记录日志
                spdlog::error("Read error: {}", ex.what());
            }
        });
}

void HttpConnection::HandleRequest()
{
    //设置版本
    _res.version(_req.version());
    //设置短连接
    _res.keep_alive(false);
    //处理get请求
    if (_req.method() == boost::beast::http::verb::get) {
        bool success = LogicSystem::get_instance()->HandleGet(_req.target(), shared_from_this());
        if (!success) {
            // 处理未知请求
            _res.result(boost::beast::http::status::not_found);
            _res.set(boost::beast::http::field::content_type, "text/plain");//设置文本类型
            boost::beast::ostream(_res.body())
                << "The resource '" << _req.target() << "' was not found.\r\n";
            WriteResponse();
            return;
        }

        // 处理成功
        _res.result(boost::beast::http::status::ok);
        _res.set(boost::beast::http::field::content_type, "GateServer");
        WriteResponse();
        return;
    }
}

void HttpConnection::WriteResponse()
{
    auto self = shared_from_this();
    // 设置内容长度 http底层会自动处理粘包
    _res.content_length(_res.body().size());
    // 异步写响应
    boost::beast::http::async_write(
        _socket,
        _res,
        [self](boost::system::error_code ec, std::size_t bytes_transferred)
        {
            self->_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
            self->_timer.cancel();
        });
}

void HttpConnection::CheckTimeout()
{
    auto self = shared_from_this();
    _timer.async_wait([self](boost::system::error_code ec)
        {
            if(!ec) {
                // 超时，关闭连接
                self->_socket.close(ec);
            }
        });
}