#pragma once

#include <asio.hpp>
#include <string>
#include "connection.hpp"
#include "connection_manager.hpp"


namespace spiritsaway::http_server
{

	/// The top-level class of the HTTP server.
	class server
	{
	public:
		server(const server &) = delete;
		server &operator=(const server &) = delete;

		/// Construct the server to listen on the specified TCP address and port, and
		/// serve up files from the given directory.
		explicit server(asio::io_context &io_context, const std::string &address, const std::string &port, const request_handler &handler);

		/// Run the server's io_context loop.
		void run();

		void stop();

		std::size_t get_connection_count();

	private:
		/// Perform an asynchronous accept operation.
		void do_accept();


		/// The io_context used to perform asynchronous operations.
		asio::io_context &io_context_;

		/// The signal_set is used to register for process termination notifications.
		asio::signal_set signals_;

		/// Acceptor used to listen for incoming connections.
		asio::ip::tcp::acceptor acceptor_;

		/// The connection manager which owns all live connections.
		connection_manager connection_manager_;

		/// The handler for all incoming requests.
		const request_handler request_handler_;

		const std::string address_;
		const std::string port_;
	};

} // namespace spiritsaway::http_server