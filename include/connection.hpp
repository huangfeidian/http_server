
#pragma once



#include <array>
#include <memory>
#include <asio.hpp>

#include "request_parser.hpp"

namespace spiritsaway::http_server{ 

class connection_manager;

/// Represents a single connection from a client.
class connection
  : public std::enable_shared_from_this<connection>
{
public:
  connection(const connection&) = delete;
  connection& operator=(const connection&) = delete;

  /// Construct a connection with the given socket.
  explicit connection(asio::ip::tcp::socket socket, connection_manager& con_mgr, const request_handler& handler);

  /// Start the first asynchronous operation for the connection.
  void start();

  /// Stop all asynchronous operations associated with the connection.
  void stop();

private:
  /// Perform an asynchronous read operation.
  void do_read();

  /// Perform an asynchronous write operation.
  void on_reply(const reply& in_reply);
  void do_write();

  void handle_request();
  void on_timeout();

  /// Socket for the connection.
  asio::ip::tcp::socket socket_;

  /// The manager for this connection.

  /// The handler used to process the incoming request.
  const request_handler request_handler_;

  /// Buffer for incoming data.
  std::array<char, 8192> buffer_;

  /// The incoming request.
  request request_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  connection_manager& connection_manager_;

  /// The reply to be sent back to the client.
  reply reply_;

  // timeout timer
  asio::basic_waitable_timer<std::chrono::steady_clock> con_timer_;
  const std::size_t timeout_seconds_ = 5;
};

typedef std::shared_ptr<connection> connection_ptr;

} 

