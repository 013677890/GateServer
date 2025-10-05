#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "CServer.h" // 包含服务器头文件
#include <boost/asio/signal_set.hpp> // 1. 包含 signal_set 头文件

int main() {
    // 确保目录存在
    std::filesystem::create_directories("log");

    // 初始化线程池：队列大小 8192，1 个后台线程（根据负载调整）
    spdlog::init_thread_pool(8192, 1);

    // 创建文件 sink（线程安全），并构造异步 logger
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("log/server.txt", false);
    auto async_logger = std::make_shared<spdlog::async_logger>(
        "async_logger",
        file_sink,
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);

    // 注册并设为默认 logger（项目其它处直接使用 spdlog::info 等）
    spdlog::register_logger(async_logger);
    spdlog::set_default_logger(async_logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] %v");
    spdlog::flush_on(spdlog::level::err);

    spdlog::info("async logger started");

    // 程序主逻辑放这里
    try {
        boost::asio::io_context ioc;

        // 2. 创建一个 signal_set 来捕获 SIGINT 和 SIGTERM 信号
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

        // 3. 异步等待信号。当信号发生时，调用 ioc.stop()
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (!error) {
                spdlog::info("Shutdown signal ({}) received. Stopping server...", signal_number);
                ioc.stop();
            }
        });

        // 创建服务器实例，监听 12345 端口
        auto server = std::make_shared<CServer>(ioc, 12345);
        server->Start();

        spdlog::info("Server started on port 12345. Press Ctrl+C to exit.");

        // 运行 io_context 事件循环，这将阻塞直到 ioc.stop() 被调用
        ioc.run();

        spdlog::info("Server stopped.");
    }
    catch (const std::exception& ex) {
        spdlog::critical("Server run failed: {}", ex.what());
    }


    // 退出前清理
    spdlog::info("Shutting down logger...");
    spdlog::shutdown();
    return 0;
}