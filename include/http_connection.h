#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <vector>

#include <asio.hpp>
using error_code = asio::error_code;

#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "http_parser.h"

namespace spiritsaway::http
{
	enum connection_state
	{
		ready,
		read_http_request_header,
		read_http_request_content,
		write_http_response_content,
		report_error,
	};
	class http_connection : public std::enable_shared_from_this <http_connection>
	{

	protected:
		asio::ip::tcp::socket client_socket;
		connection_state _con_state;
		std::unordered_map<std::string, std::shared_ptr<asio::basic_waitable_timer<std::chrono::steady_clock>>> timers;
		std::array<unsigned char, MAX_HTTP_BUFFER_LENGTH> client_read_buffer;

		std::array<unsigned char, MAX_HTTP_BUFFER_LENGTH> client_send_buffer;
		std::chrono::seconds timeout;
		std::shared_ptr<spdlog::logger> logger;

		decltype(std::chrono::system_clock::now()) _request_time;
	public:
		const std::uint32_t connection_count;
		const std::string logger_prefix;
		
	protected:

		// trace header
		http_request_parser _request_parser;
		std::string response_str;
	public:
		virtual ~http_connection();
		static std::shared_ptr<http_connection> create( asio::ip::tcp::socket&& _in_client_socket,  std::shared_ptr<spdlog::logger> logger, std::uint32_t in_connection_idx, std::uint32_t _in_timeout, std::string log_pre);
		http_connection(asio::ip::tcp::socket&& in_client_socket, std::shared_ptr<spdlog::logger> logger, std::uint32_t in_connection_idx, std::uint32_t in_timeout,  std::string log_pre = "connection");
		virtual void start();
	protected:
		virtual void async_read_data_from_client(bool set_timer = true, std::size_t at_least_size = 1, std::size_t at_most_size = BUFFER_LENGTH);
		virtual void async_send_data_to_client(const unsigned char* write_buffer, std::size_t offset, std::size_t size);
		virtual void async_send_data_to_client_impl(const unsigned char* write_buffer, std::size_t offset, std::size_t remain_size, std::size_t total_size);
		virtual void on_client_data_header_read(const http_request_header& _header);
		virtual void on_client_data_body_read(const http_request_header& _header, std::string_view content);
		void set_timer(std::string _cur_timer);
		bool cancel_timer(std::string _cur_timer);
		void cancel_all_timers();
		virtual void on_error(const error_code& error);
		virtual void on_timeout(std::string _cur_timer_type);
	protected:
		// trace header
		virtual void on_client_data_arrived(std::size_t bytes_transfered);
		virtual void on_client_data_send(std::size_t bytes_transfered);
		virtual void report_error(reply_status _error_status, const std::string& 
		status_description, const std::string& error_message);
		void close_connection();
		virtual void report_error(http_parser_result _status);

	};

} // namespace http_proxy