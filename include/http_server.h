#pragma once

#include <atomic>
#include <asio.hpp>
using error_code = asio::error_code;

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace spiritsaway::http
{
	enum class listen_mode
	{
		local_v4 = 0,
		public_v4 ,
		local_v6,
		public_v6,
	};
	template<typename T, typename D = void, listen_mode _mode = listen_mode::local_v4>
	class http_server
	{
		asio::io_context& io_context;
		asio::ip::tcp::acceptor acceptor;
		std::shared_ptr<spdlog::logger> logger;
		std::uint16_t port;
		std::uint32_t connection_idx = 0;
		const std::string server_name;
	protected:
		D* _common_data;

	public:

		http_server(asio::io_context& io_context, const std::string in_server_name, std::uint16_t in_port, D* _in_common_data = nullptr) :
			io_context(io_context),
			acceptor(io_context),
			port(in_port),
			server_name(in_server_name),
			_common_data(_in_common_data)
			
		{
		}

		void run()
		{
			std::string listen_ip = "127.0.0.1";
			switch (_mode)
			{
			case spiritsaway::http::listen_mode::local_v4:
				listen_ip = "127.0.0.1";
				break;
			case spiritsaway::http::listen_mode::public_v4:
				listen_ip = "0.0.0.0";
				break;
			case spiritsaway::http::listen_mode::local_v6:
				listen_ip = "::1";
				break;
			case spiritsaway::http::listen_mode::public_v6:
				listen_ip = "::";
				break;
			default:
				break;
			}
			asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(listen_ip), port);
			this->acceptor.open(endpoint.protocol());
			this->acceptor.bind(endpoint);
			this->acceptor.listen(asio::socket_base::max_connections);
			time_t rawtime;
			struct tm * timeinfo;
			char buffer[80];

			time(&rawtime);
			timeinfo = localtime(&rawtime);

			strftime(buffer, sizeof(buffer), "%d-%m-%Y-%H-%M-%S", timeinfo);
			std::string log_file_name = server_name + "-" + std::string(buffer) + ".log";
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(spdlog::level::debug);
			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_name, true);
			file_sink->set_level(spdlog::level::debug);
			this->logger = std::make_shared<spdlog::logger>(server_name, spdlog::sinks_init_list{ console_sink, file_sink });
			this->logger->set_level(spdlog::level::debug);
			this->logger->flush_on(spdlog::level::warn);
			this->start_accept();
		}
	public:
		void start_accept()
		{
			auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
			this->acceptor.async_accept(*socket, [socket, this](const error_code& error)
			{
				if (!error)
				{
					this->start_accept();

					auto connection = T::create(std::move(*socket), this->logger, connection_idx++, 10, "connection", reinterpret_cast<void*>(_common_data));

					connection->start();
				}
			});
		}
	};

} // namespace http_proxy

