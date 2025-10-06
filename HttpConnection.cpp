#include "HttpConnection.h"
#include "LogicSystem.h"

//下面是关于url解析的内容
//十进制转十六进制
unsigned char Tollex(unsigned char x)
{
    return x>9? (x-10+'A'):(x+'0');
}
//十六进制转十进制
unsigned char ToDeclex(unsigned char x)
{
    if(x>='0' && x<='9') return x-'0';
    if(x>='a' && x<='f') return x-'a'+10;
    if(x>='A' && x<='F') return x-'A'+10;
    return 0;
}
//url编码
std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for(size_t i = 0; i < length; i++)
    {
        if(isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||    
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += Tollex((unsigned char)str[i] >> 4);
            strTemp += Tollex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}
//url解码
std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for(size_t i = 0; i < length; i++)
    {
        if(str[i] == '+') strTemp += ' ';
        else if(str[i] == '%')
        {
            if(i + 2 < length)
            {
                unsigned char high = ToDeclex((unsigned char)str[i + 1]);
                unsigned char low = ToDeclex((unsigned char)str[i + 2]);
                strTemp += high*16 + low;
                i += 2;
            }
        }
        else strTemp += str[i];
    }
    return strTemp;
}
//获取路径
std::string GetPath(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for(size_t i = 0; i < length; i++)
    {
        if(str[i] == '?') break;
        strTemp += str[i];
    }
    return strTemp;
}



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
                    spdlog::error("读取请求时发生错误: {}", ec.message());
                    return;
                }
                boost::ignore_unused(bytes_transferred);//http服务器可以忽略已发送的字节数（不需要粘包处理
                self->HandleRequest();
                //启动超时检测
                self->CheckTimeout();
            }
            catch (const std::exception& ex) {
                // 处理异常，记录日志
                spdlog::error("读取请求时发生错误: {}", ex.what());
            }
        });
}

void HttpConnection::HandleRequest()
{
    // 设置一些所有响应通用的默认值
    _res.version(_req.version());
    _res.keep_alive(false);

    // 提前获取路径
    std::string path = GetPath(std::string(_req.target()));
    bool success = false;

    // 根据请求方法进行分发
    if (_req.method() == boost::beast::http::verb::get) {
        ParseUrlParams(); // 只在 GET 请求时解析 URL 参数
        success = LogicSystem::get_instance()->HandleGet(path, shared_from_this());
    }
    else if (_req.method() == boost::beast::http::verb::post) {
        success = LogicSystem::get_instance()->HandlePost(path, shared_from_this());
    }
    else {
        // 处理其他不支持的请求方法，例如 PUT, DELETE 等
        _res.result(boost::beast::http::status::method_not_allowed);
        _res.set(boost::beast::http::field::content_type, "text/plain");
        boost::beast::ostream(_res.body()) << "Method Not Allowed\r\n";
        // 直接发送响应，不进入下面的逻辑
        WriteResponse();
        return;
    }

    // 统一处理路由查找结果
    if (!success) {
        // 如果 success 为 false，说明 LogicSystem 没有找到对应的处理器
        // 这是唯一需要 HttpConnection 自己决定响应内容的情况
        _res.result(boost::beast::http::status::not_found);
        _res.set(boost::beast::http::field::content_type, "text/plain");
        boost::beast::ostream(_res.body())
            << "The resource '" << _req.target() << "' was not found.\r\n";
    }

    // 如果 success 为 true，说明 LogicSystem 中的处理器已经构建好了完整的 _res
    // 我们什么都不用做，直接发送即可

    WriteResponse();
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
                spdlog::info("连接超时，已关闭。");
            }
        });
}

//解析url参数
void HttpConnection::ParseUrlParams()
{
    _url_params.clear();
    std::string target = std::string(_req.target());
    size_t pos = target.find('?');
    if (pos == std::string::npos) {
        return; // 没有参数
    }
    std::string query = target.substr(pos + 1);
    size_t start = 0;
    while (start < query.length()) {
        size_t end = query.find('&', start);
        if (end == std::string::npos) {
            end = query.length();
        }
        size_t eq_pos = query.find('=', start);
        if (eq_pos != std::string::npos && eq_pos < end) {
            std::string key = UrlDecode(query.substr(start, eq_pos - start));
            std::string value = UrlDecode(query.substr(eq_pos + 1, end - eq_pos - 1));
            _url_params[key] = value;
        }
        start = end + 1;
    }
    //调试输出参数
    for (const auto& param : _url_params) {
        spdlog::info("URL Param: {} = {}", param.first, param.second);
    }
}