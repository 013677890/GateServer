# pragma once

#include "global.h"

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& ioc, unsigned short port);
    void Start();
private:
    // 监听器对象
    boost::asio::ip::tcp::acceptor _acceptor;
    //io_context 对象的引用
    boost::asio::io_context& _ioc;
    //socket 对象
    boost::asio::ip::tcp::socket _socket;
};