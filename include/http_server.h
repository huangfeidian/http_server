#pragma once

#include <atomic>
#include <asio.hpp>
using error_code = asio::error_code;

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace spiritsaway::http
{

	template<typename T, typename D = void>
	class http_server
	{
		asio::io_context& io_context;
		asio::ip::tcp::acceptor acceptor;
		std::shared_ptr<spdlog::logger> logger;
		std::uint16_t port;
		std::uint8_t worker_size;
		std::uint32_t connection_idx = 0;
		const std::string server_name;
	protected:
		D* _common_data;

	public:

		http_server(asio::io_context& io_context, const std::string in_server_name, std::uint16_t in_port, std::uint8_t in_worker_size, D* _in_common_data = nullptr) :
			io_context(io_context),
			acceptor(io_context),
			port(in_port),
			worker_size(in_worker_size ? in_worker_size: 1),
			server_name(in_server_name),
			_common_data(_in_common_data)
			
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

			strftime(buffer, sizeof(buffer), "%d-%m-%Y-%H-%M-%S", timeinfo);
			std::string log_file_name = server_name + "-" + std::string(buffer) + ".log";
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(spdlog::level::debug);
			auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_name, true);
			file_sink->set_level(spdlog::level::debug);
			this->logger = std::make_shared<spdlog::logger>(server_name, spdlog::sinks_init_list{ console_sink, file_sink });
			this->logger->set_level(spdlog::level::debug);
			this->logger->flush_on(spdlog::level::warn);
			this->logger->info("{}  runs with {} threads", server_name, worker_size);
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

