
#include "connection.hpp"
#include <utility>
#include <vector>
#include "connection_manager.hpp"
#include <iostream>

namespace spiritsaway::http_server {

	connection::connection(asio::ip::tcp::socket socket, connection_manager& con_mgr, const request_handler& handler)
		: socket_(std::move(socket)),
		connection_manager_(con_mgr),
		request_handler_(handler),
		con_timer_(socket_.get_executor())
	{
		std::cout << "new connection begin" << std::endl;
	}

	void connection::start()
	{
		do_read();
	}

	void connection::stop()
	{
		socket_.close();
	}

	void connection::do_read()
	{
		auto self(shared_from_this());

		if (con_timer_.expires_from_now(std::chrono::seconds(timeout_seconds_)) != 0)
		{
			connection_manager_.stop(self);
			return;
		}

		socket_.async_read_some(asio::buffer(buffer_),
			[this, self](std::error_code ec, std::size_t bytes_transferred)
			{
				con_timer_.cancel();

				if (!ec)
				{
					request_parser::result_type result = request_parser_.parse(buffer_.data(), bytes_transferred);

					if (result == request_parser::result_type::good)
					{
						handle_request();
					}
					else if (result == request_parser::result_type::bad)
					{
						reply_ = reply::stock_reply(reply::status_type::bad_request);
						do_write();
					}
					else
					{
						do_read();
					}
				}
				else if (ec != asio::error::operation_aborted)
				{
					connection_manager_.stop(shared_from_this());
				}
			});
	}

	void connection::do_write()
	{
		auto self(shared_from_this());

		if (con_timer_.expires_from_now(std::chrono::seconds(timeout_seconds_)) != 0)
		{
			connection_manager_.stop(self);
			return;
		}
		m_reply_str = reply_.to_string();
		asio::async_write(socket_, asio::buffer(m_reply_str),
			[this, self](std::error_code ec, std::size_t)
			{
				if (!ec)
				{
					// Initiate graceful connection closure.
					asio::error_code ignored_ec;
					socket_.shutdown(asio::ip::tcp::socket::shutdown_both,
						ignored_ec);
				}

				if (ec != asio::error::operation_aborted)
				{
					connection_manager_.stop(shared_from_this());
				}
			});
	}

	void connection::on_reply(const reply& in_reply)
	{
		con_timer_.cancel();
		reply_ = in_reply;
		do_write();

	}
	void connection::on_timeout()
	{
		connection_manager_.stop(shared_from_this());
	}
	void connection::handle_request()
	{
		auto self = shared_from_this();
		request_ = std::make_shared<request>();
		request_parser_.move_req(*request_);
		if (con_timer_.expires_from_now(std::chrono::seconds(timeout_seconds_)) != 0)
		{
			connection_manager_.stop(self);
			return;
		}
		auto weak_self = std::weak_ptr<connection>(self);
		auto weak_request = std::weak_ptr<request>(request_);
		request_handler_(weak_request, [weak_self](const reply& in_reply) {
			auto strong_self = weak_self.lock();
			if (strong_self)
			{
				strong_self->on_reply(in_reply);
			}
			});
	}
}
