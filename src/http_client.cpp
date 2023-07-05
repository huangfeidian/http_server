#include "http_client.h"
#include <sstream>

namespace spiritsaway::http_server
{
	http_client::http_client(asio::io_context &io_context, const std::string &server_url, const std::string &server_port, const request &req, std::function<void(const std::string &, const reply &)> callback, std::uint32_t timeout_second)
		: m_socket(io_context), m_resolver(io_context), m_callback(callback)
		, m_req_str(req_to_str(req, server_url, server_port))
		, m_timer(io_context)
		, m_timeout_seconds(timeout_second)
		, m_server_url(server_url)
		, m_server_port(server_port)
	{
		

	}
	std::string http_client::req_to_str(const request& req, const std::string& server_url, const std::string& server_port)
	{
		std::ostringstream request_stream;
		request_stream << req.method << " " << req.uri << " HTTP/" << req.http_version_major << "." << req.http_version_minor << "\r\n";
		request_stream << "Host: " << server_url <<"\r\n";
		request_stream << "Accept: */*\r\n";
		for (const auto &one_header : req.headers)
		{
			request_stream << one_header.name << ": " << one_header.value << "\r\n";
		}
		request_stream << "Connection: close\r\n";
		request_stream << "Content-Length: " << req.body.size() << "\r\n\r\n";
		request_stream << req.body;
		return request_stream.str();
	}

	void http_client::run()
	{
		auto self = shared_from_this();
		asio::ip::tcp::resolver::query query(m_server_url, m_server_port);
		m_resolver.async_resolve(query, [self, this](const asio::error_code& error, asio::ip::tcp::resolver::iterator iterator)
								{ handle_resolve(error, iterator); });
		m_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds));
		m_timer.async_wait([self, this](const asio::error_code& error)
		{
			on_timeout(error);
		});
	}

	void http_client::handle_resolve(const asio::error_code& error, asio::ip::tcp::resolver::iterator iterator)
	{
		if (error)
		{

			invoke_callback(error.message());
			return;
		}
		auto self = shared_from_this();
		m_socket.async_connect(*iterator, [self, this](const asio::error_code &err)
							{ handle_connect(err); });
	}

	void http_client::handle_connect(const asio::error_code &err)
	{
		if (err)
		{

			invoke_callback(err.message());
			return;
		}

		auto self = shared_from_this();
		asio::async_write(m_socket, asio::buffer(m_req_str), [self, this](const asio::error_code &err, std::size_t write_sz)
						  { handle_write_request(err); });
	}
	void http_client::handle_write_request(const asio::error_code &err)
	{
		if (err)
		{

			invoke_callback(err.message());
			return;
		}
		m_socket.async_read_some(asio::buffer(m_content_read_buffer.data(), m_content_read_buffer.size()), [self = shared_from_this(), this](const asio::error_code& err, std::size_t n)
		{
			handle_read_content(err, n);
		});
	}
	
	void http_client::handle_read_content(const asio::error_code &err, std::size_t n)
	{
		if(err)
		{
			if(err == asio::error::eof)
			{
				m_reply.content = reply_oss.str();
				invoke_callback("");
			}
			else
			{
				invoke_callback(err.message());
			}
			return;
		}
		std::string temp_content(m_content_read_buffer.data(), n);
		std::cout << "read content: " << temp_content << std::endl;
		auto temp_parse_result = m_rep_parser.parse(m_content_read_buffer.data(), n);
		if (temp_parse_result == reply_parser::result_type::bad)
		{
			invoke_callback("invalid reply");
			return;
		}
		m_socket.async_read_some(asio::buffer(m_content_read_buffer.data(), m_content_read_buffer.size()),  [self=shared_from_this(), this](const asio::error_code& err, std::size_t bytes_transferred)
		{
			handle_read_content(err, bytes_transferred);
		});

	}
	void http_client::invoke_callback(const std::string& err)
	{
		m_timer.cancel();
		m_callback(err, m_reply);
		m_socket.close();

	}
	
	void http_client::on_timeout(const asio::error_code& err)
	{
		if(err != asio::error::operation_aborted)
		{
			invoke_callback("timeout");
		}
	}

	std::string http_client::parse_uri(const std::string& full_path, std::string& server_url, std::string& server_port, std::string& resource_path)
	{
		std::string_view path_view(full_path);
		if(path_view.find("https") == 0)
		{
			return "https not supported";
		}
		static const std::string http_prefix = "http://";
		if(path_view.find(http_prefix) == 0)
		{
			path_view.remove_prefix(http_prefix.size());
		}
		auto resource_iter = path_view.find("/");
		if(resource_iter == std::string_view::npos)
		{
			resource_path = "/";
		}
		else
		{
			resource_path = path_view.substr(resource_iter);
			path_view = path_view.substr(0, resource_iter);
		}
		
		auto port_iter = path_view.find(":");
		if(port_iter == std::string_view::npos)
		{
			server_port = "80";
			server_url = path_view;
		}
		else
		{
			server_port = path_view.substr(port_iter + 1);
			server_url = path_view.substr(0, port_iter);
		}
		return {};
	}


}