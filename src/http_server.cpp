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
		// Register to handle the signals that indicate when the server should exit.
		// It is safe to register for the same signal multiple times in a program,
		// provided all registration for the specified signal is made through Asio.
//		signals_.add(SIGINT);
//		signals_.add(SIGTERM);
//#if defined(SIGQUIT)
//		signals_.add(SIGQUIT);
//#endif // defined(SIGQUIT)

		do_await_stop();
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

	void server::do_await_stop()
	{
		signals_.async_wait(
			[this](std::error_code /*ec*/, int /*signo*/) {
				// The server is stopped by cancelling all outstanding asynchronous
				// operations. Once all operations have finished the io_context::run()
				// call will exit.
				acceptor_.close();
				connection_manager_.stop_all();
			});
	}

} // namespace spiritsaway::http_server
