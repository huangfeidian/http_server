#pragma once

#include <atomic>
#include <asio.hpp>
using error_code = asio::error_code;

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace spiritsaway::http
{

	template<typename T>
	class http_server
	{
		asio::io_context& io_context;
		asio::ip::tcp::acceptor acceptor;
		std::shared_ptr<spdlog::logger> logger;
		std::uint16_t port;
		std::uint8_t worker_size;
		std::uint32_t connection_idx = 0;
	public:

		http_server(asio::io_context& io_context, std::uint16_t in_port, std::uint8_t in_worker_size) :
			io_context(io_context),
			acceptor(io_context),
			port(in_port),
			worker_size(in_worker_size ? in_worker_size: 1)
			
		{
		}

		void run()
		{
			asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string("127.0.0.1"), port);
			this->acceptor.open(endpoint.protocol());
			this->acceptor.bind(endpoint);
			this->acceptor.listen(asio::socket_base::max_connections);
			time_t rawtime;
			struct tm * timeinfo;
			char buffer[80];

			time(&rawtime);
			timeinfo = localtime(&rawtime);

			strftime(buffer, sizeof(buffer), "%d-%m-%Y-%H:%M:%S", timeinfo);
			//std::string log_file_name = "http_server-" + std::string(buffer) + ".log";
			std::string log_file_name = "http_server.log";
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(spdlog::level::debug);
			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_name, true);
			file_sink->set_level(spdlog::level::debug);
			this->logger = std::make_shared<spdlog::logger>("http_server", spdlog::sinks_init_list{ console_sink, file_sink });
			this->logger->set_level(spdlog::level::debug);
			this->logger->flush_on(spdlog::level::warn);
			this->logger->info("http_server  runs with {} threads", worker_size);
			this->start_accept();
			std::vector<std::thread> td_vec;
			for (auto i = 0; i < worker_size; ++i)
			{
				td_vec.emplace_back([this]()
				{
					try
					{
						this->io_context.run();
					}
					catch (const std::exception& e)
					{
						std::cerr << e.what() << std::endl;
					}
				});
			}

			for (auto& td : td_vec)
			{
				td.join();
			}
		}
	private:
		void start_accept()
		{
			auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
			this->acceptor.async_accept(*socket, [socket, this](const error_code& error)
			{
				if (!error)
				{
					this->start_accept();

					auto connection = T::create(std::move(*socket), this->logger, connection_idx++, 10, "connection");

					connection->start();
				}
			});
		}
	};

} // namespace http_proxy

