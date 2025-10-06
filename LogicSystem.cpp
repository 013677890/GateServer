#include "LogicSystem.h"
#include "HttpConnection.h" //不在头文件中包含，避免循环依赖

LogicSystem::~LogicSystem()
{
}

LogicSystem::LogicSystem()
{
}

bool LogicSystem::Init()
{
    spdlog::info("LogicSystem 初始化");
    if (_isInited) {
        spdlog::warn("LogicSystem 已经初始化过，重复初始化被忽略。");
        return true;
    }
    // 注册 GET 路由
    RegGet("/get_test", std::bind(&LogicSystem::Handle_GetTest, this
        , std::placeholders::_1));//注册 /get_test 路由

    //注册 Post路由
    RegPost("/get_varifycode", std::bind(&LogicSystem::Handle_GetVarifyCode, this
        , std::placeholders::_1));//注册 /get_varifycode 路由
    return true;
}

bool LogicSystem::HandleGet(std::string path, const std::shared_ptr<HttpConnection> conn)
{
    auto it = _getHandlers.find(path);
    if (it != _getHandlers.end())
    {
        // 找到处理函数，调用它
        it->second(conn);
        return true;
    }
    return false; // 未找到处理函数
}

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
    _getHandlers.insert(std::make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
    _postHandlers.insert(std::make_pair(url, handler));
}

bool LogicSystem::HandlePost(std::string path, const std::shared_ptr<HttpConnection> conn)
{
    auto it = _postHandlers.find(path);
    if (it != _postHandlers.end())
    {
        // 找到处理函数，调用它
        it->second(conn);
        return true;
    }
    return false; // 未找到处理函数
}

void LogicSystem::Handle_GetTest(const std::shared_ptr<HttpConnection> conn)
{
    boost::beast::ostream(conn->_res.body())
        << "This is a test response from /get_test endpoint.\r\n";
    int i = 0;
    for (const auto& param : conn->_url_params) {
        boost::beast::ostream(conn->_res.body())
            << "Param " << i++ << ": " << param.first << " = " << param.second << "\r\n";
    }
    conn->_res.result(boost::beast::http::status::ok);
}

void LogicSystem::Handle_GetVarifyCode(const std::shared_ptr<HttpConnection> conn)
{
    auto body_str = boost::beast::buffers_to_string(conn->_req.body().data());
    spdlog::info("收到 /get_varifycode 请求,body: {}", body_str);
    conn->_res.set(boost::beast::http::field::content_type, "application/json");

    Json::Value res_root;
    Json::Value src_root;
    Json::Reader reader;

    if (!reader.parse(body_str, src_root)) {
        res_root["error"] = ERROR_JSON;
        res_root["message"] = "JSON format error.";
        conn->_res.result(boost::beast::http::status::bad_request);
    } else {
        // 1. 服务端校验：检查字段存在性和类型
        if (!src_root.isMember("email") || !src_root["email"].isString()) {
            res_root["error"] = ERROR_JSON;
            res_root["message"] = "Field 'email' is missing or not a string.";
            conn->_res.result(boost::beast::http::status::bad_request);
        } else {
            std::string email = src_root["email"].asString();
            spdlog::info("解析到 email: {}", email);

            // 2. 服务端校验：检查 email 格式 (这里用一个简单的模拟函数)
            if (email.empty() || email.find('@') == std::string::npos) {
                res_root["error"] = ERROR_JSON;
                res_root["message"] = "Invalid email format.";
                conn->_res.result(boost::beast::http::status::bad_request);
            } else {
                // 3. 处理外部服务调用
                std::string code = "123456"; // 模拟生成的验证码

                // 模拟调用邮件服务，它可能成功也可能失败
                bool email_sent_successfully = true; //  <-- 替换为真实的邮件发送函数，如: SendEmail(email, code);

                if (email_sent_successfully) {
                    spdlog::info("发送验证码 {} 到邮箱 {} 成功", code, email);
                    res_root["error"] = ERROR_SUCCESS;
                    res_root["message"] = "验证码发送成功";
                    conn->_res.result(boost::beast::http::status::ok);
                } else {
                    spdlog::error("发送验证码到邮箱 {} 失败", email);
                    res_root["error"] = ERROR_RPCFAILED; // 定义一个新的错误码
                    res_root["message"] = "Failed to send verification code. Please try again later.";
                    conn->_res.result(boost::beast::http::status::internal_server_error); // 使用 500 状态码表示服务端内部错误
                }
            }
        }
    }
    std::string jsonstr = res_root.toStyledString();
    boost::beast::ostream(conn->_res.body()) << jsonstr;
}