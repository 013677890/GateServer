#include "CServer.h"
#include "HttpConnection.h" 
CServer::CServer(boost::asio::io_context& ioc, unsigned short port)
    : _acceptor(ioc, { boost::asio::ip::tcp::v4(), port })
    , _ioc(ioc)
    , _socket(ioc)
{
}

void CServer::Start()
{
    // 开始异步接受连接
    _acceptor.async_accept(
        _socket,
        [self = shared_from_this()](boost::system::error_code ec)
        {
            try {
                if (ec) {
                    spdlog::error("接受连接失败: {}", ec.message());
                    self->Start(); // 继续接受下一个连接
                    return;
                }
                // 创建并启动新的连接处理对象
                std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
                spdlog::info("新连接已接受。");
                self->Start(); // 继续接受下一个连接
            }
            catch (const std::exception& ex) {
                // 处理异常，记录日志
                spdlog::error("接受连接时发生错误: {}", ex.what());
            }
        });
}