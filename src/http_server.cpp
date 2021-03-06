//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "http_server.hpp"
#include <signal.h>
#include <utility>

namespace spiritsaway::http_server
{

	server::server(asio::io_context &io_context, const std::string &address, const std::string &port, const request_handler &handler)
		: io_context_(io_context),
		  signals_(io_context_),
		  acceptor_(io_context_),
		  connection_manager_(),
		  request_handler_(handler),
		  address_(address),
		  port_(port)
	{

	}

	void server::run()
	{
		// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
		asio::ip::tcp::resolver resolver(io_context_);
		asio::ip::tcp::endpoint endpoint =
			*resolver.resolve(address_, port_).begin();
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();
		do_accept();
	}

	void server::do_accept()
	{
		acceptor_.async_accept(
			[this](std::error_code ec, asio::ip::tcp::socket socket) {
				// Check whether the server was stopped by a signal before this
				// completion handler had a chance to run.
				if (!acceptor_.is_open())
				{
					return;
				}

				if (!ec)
				{
					connection_manager_.start(std::make_shared<connection>(
						std::move(socket), connection_manager_, request_handler_));
				}

				do_accept();
			});
	}


	void server::stop()
	{
		acceptor_.close();
		connection_manager_.stop_all();
	}

	std::size_t server::get_connection_count()
	{
		return connection_manager_.get_connection_count();
	}
} // namespace spiritsaway::http_server
