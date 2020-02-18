#include <http_connection.h>

using namespace spiritsaway::http;
http_connection::http_connection(asio::ip::tcp::socket&& in_client_socket, std::shared_ptr<spdlog::logger> in_logger, std::size_t in_connection_count, std::uint32_t in_timeout, std::string log_pre)
:
client_socket(std::move(in_client_socket)),

_con_state(connection_state::ready),
timeout(std::chrono::seconds(in_timeout)),
logger(in_logger),
connection_count(in_connection_count),
logger_prefix(log_pre + " " +std::to_string(in_connection_count) + ": ")
{
	
}
std::shared_ptr<http_connection> http_connection::create(asio::ip::tcp::socket&& _in_client_socket, std::shared_ptr<spdlog::logger> logger, std::uint32_t in_connection_idx, std::uint32_t _in_timeout, std::string log_pre)
{
	return std::make_shared<http_connection>(std::move(_in_client_socket), logger, in_connection_idx, _in_timeout, log_pre);
}

void http_connection::start()
{

		this->async_read_data_from_client(true, 1, BUFFER_LENGTH);
		logger->info("{} new connection start", logger_prefix);
}
void http_connection::close_connection()
{
	error_code ec;

	if (this->client_socket.is_open())
	{
		this->client_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		this->client_socket.close(ec);
	}
}
void http_connection::on_timeout(std::string _cur_timer_type)
{
	logger->info("{} on_timeout for timer {}", logger_prefix, _cur_timer_type);
	logger->warn("{} on_timeout shutdown connection", logger_prefix);
		close_connection();
}
void http_connection::on_error(const error_code& error)
{
	logger->warn("{} error shutdown connection {}", logger_prefix, error.message());
	this->cancel_all_timers();
	close_connection();
}
void http_connection::cancel_all_timers()
{
	logger->warn("{} cancel all timers", logger_prefix);
	int i = 0;
	for (auto& one_timer : timers)
	{
		std::size_t ret = one_timer.second->cancel();
		if (ret > 1)
		{
			logger->error("{} cancel_timer {} fail", logger_prefix, one_timer.first);
		}
		
		assert(ret <= 1);
	}
	return;
}
bool http_connection::cancel_timer(std::string _cur_timer_type)
{
	logger->debug("{} cancel_timer {}", logger_prefix, _cur_timer_type);
	auto cur_timer_iter = timers.find(_cur_timer_type);
	if (cur_timer_iter == timers.end())
	{
		return false;
	}
	auto& cur_timer = cur_timer_iter->second;
	std::size_t ret = cur_timer->cancel();
	if (ret > 1)
	{
		logger->error("{} cancel_timer {} fail", logger_prefix, _cur_timer_type);
	}
	assert(ret <= 1);
	return ret <= 1;
}
void http_connection::set_timer(std::string _cur_timer_type)
{
	logger->debug("{} set_timer {}", logger_prefix, _cur_timer_type);
	auto cur_timer_iter = timers.find(_cur_timer_type);
	if (cur_timer_iter != timers.end())
	{
		auto& cur_timer = cur_timer_iter->second;
		if (cur_timer->expires_from_now(this->timeout) != 0)
		{
			logger->error("{} set_timer {} fail", logger_prefix, _cur_timer_type);
			assert(false);
		}
		return;
	}

	auto self(this->shared_from_this());
	auto cur_timer = std::make_shared< asio::basic_waitable_timer<std::chrono::steady_clock>>(client_socket.get_executor());
	if (cur_timer->expires_from_now(std::chrono::milliseconds(timeout)) != 0)
	{
		logger->error("{} set_timer {} fail", logger_prefix, _cur_timer_type);
		assert(false);
		return;
	}
	timers[_cur_timer_type] = cur_timer;
	cur_timer->async_wait([this, self, _cur_timer_type](const error_code& error)
	{
		if (error != asio::error::operation_aborted)
		{
			this->on_timeout(_cur_timer_type);
		}
	});
}

void http_connection::async_read_data_from_client(bool set_timer, std::size_t at_least_size, std::size_t at_most_size)
{
	logger->debug("{} async_read_data_from_client at_least_size {} at_most_size {}", logger_prefix, at_least_size, at_most_size);
	assert(at_least_size <= at_most_size && at_most_size <= BUFFER_LENGTH);
	auto self(this->shared_from_this());
	if(set_timer)
	{
		this->set_timer("client_read");
	}
	
	asio::async_read(this->client_socket,
							asio::buffer(&this->client_read_buffer[0], at_most_size),
							asio::transfer_at_least(at_least_size),
							[this, self](const error_code& error, std::size_t bytes_transferred)
	{
		if (this->cancel_timer("client_read"))
		{
			if (!error)
			{
				this->on_client_data_arrived(bytes_transferred);
			}
			else
			{
				logger->warn("{} report error at {}", logger_prefix, "async_read_data_from_client");
				this->on_error(error);
			}
		}
	});
}



void http_connection::async_send_data_to_client(const unsigned char* write_buffer, std::size_t offset, std::size_t size)
{
	async_send_data_to_client_impl(write_buffer, offset, size, size);
}
void http_connection::async_send_data_to_client_impl(const unsigned char* write_buffer, std::size_t offset, std::size_t remain_size, std::size_t total_size)
{
	auto self(this->shared_from_this());
	this->set_timer("client_send");

	this->client_socket.async_write_some(asio::buffer(write_buffer + offset, remain_size),
		[this, self, write_buffer, offset, remain_size, total_size](const error_code& error, std::size_t bytes_transferred)
	{
		if (this->cancel_timer("client_send"))
		{
			if (!error)
			{
				if (bytes_transferred < remain_size)
				{
					logger->debug("{} send to client with bytes transferred {}", logger_prefix, bytes_transferred);
					this->async_send_data_to_client_impl(write_buffer, offset + bytes_transferred, remain_size - bytes_transferred, total_size);
				}
				else
				{
					logger->debug("{} send to client with size {}", logger_prefix, total_size);
					response_str.clear();
					this->on_client_data_send(total_size);
				}
			}
			else
			{
				logger->warn("{} report error at {}", logger_prefix, "async_send_data_to_client");
				this->on_error(error);
			}
		}
	});

}

http_connection::~http_connection()
{
	close_connection();
}
void http_connection::on_client_data_arrived(std::size_t bytes_transferred)
{
	logger->debug("{} on_client_data_arrived size {} proxy_connection_state {}", logger_prefix, bytes_transferred, _con_state);
	
	logger->debug("{} data is {}", logger_prefix, std::string(reinterpret_cast<char*>(client_read_buffer.data()), bytes_transferred));

	if (!_request_parser.append_input(client_read_buffer.data(), bytes_transferred))
	{
		report_error(reply_status::bad_request, "Bad Request", "buffer overflow");
	}
	
	std::uint32_t send_buffer_size = 0;

	bool header_readed = false;
	while (true)
	{
		auto cur_parse_result = _request_parser.parse();
		if (cur_parse_result.first >= http_parser_result::parse_error)
		{
			report_error(cur_parse_result.first);
			return;
		}
		else if (cur_parse_result.first == http_parser_result::read_one_header)
		{
			_request_time = std::chrono::system_clock::now();


			header_readed = true;
			on_client_data_header_read();
		}
		else if (cur_parse_result.first == http_parser_result::read_some_content)
		{
			continue;
		}
		else if (cur_parse_result.first == http_parser_result::read_content_end)
		{
			on_client_data_body_read();
		}
		else
		{
			break;
		}
	}
	if (response_str.size())
	{
		async_send_data_to_client(reinterpret_cast<unsigned char *>(response_str.data()), 0, response_str.size());
	}
	else
	{
		async_read_data_from_client();
	}
	
}
void http_connection::on_client_data_send(std::size_t bytes_transferred)
{
	async_read_data_from_client();
}
void http_connection::on_client_data_header_read()
{
	auto header_data = _request_parser._header.encode_to_data();
	logger->trace("{} read client request header {}", logger_prefix, header_data);
	return;
}
void http_connection::on_client_data_body_read()
{
	auto header_data = _request_parser._header.encode_to_data();
	logger->trace("{} read client request body read  header {}", logger_prefix, header_data);
	async_read_data_from_client();
	return;
}



void http_connection::report_error(reply_status status_code, const std::string& status_description, const std::string& error_message)
{
	logger->warn("{} report error status_code {} status_description {} error_message {}", logger_prefix, status_code, status_description, error_message);
	http_response_header _cur_response;
	_cur_response.set_version("1.1");
	_cur_response.status_code(status_code);
	_cur_response.status_description(status_description);

	_cur_response.add_header_value("Content-Type", "text/html");
	_cur_response.add_header_value("Server", "Http Server");

	std::string response_content;
	response_content = "<!DOCTYPE html><html><head><title>";
	response_content += std::to_string(static_cast<int>(status_code));
	response_content += ' ';
	response_content += status_description;
	response_content += "</title></head><body bgcolor=\"white\"><center><h1>";
	response_content += std::to_string(static_cast<int>(status_code));
	response_content += ' ';
	response_content += status_description;
	response_content += "</h1>";
	if (!error_message.empty())
	{
		response_content += "<br/>";
		response_content += error_message;
		response_content += "</center>";
	}
	response_content += "<hr><center>";
	response_content += "Http Server";
	response_content += "</center></body></html>";
	
	if (_request_parser._header.method() != "HEAD")
	{
		_cur_response.add_header_value("Content-Length", std::to_string(response_content.size()));
		response_str = _cur_response.encode_to_data();
	}
	else
	{
		response_str = _cur_response.encode_to_data(response_content);
	}
	
	_con_state = connection_state::report_error;
	auto self(this->shared_from_this());
	this->async_send_data_to_client(reinterpret_cast<unsigned char*>(response_str.data()), 0, response_str.size());
}

void http_connection::report_error(http_parser_result _status)
{
	auto error_detail = from_praser_result_to_description(_status);
	report_error(std::get<0>(error_detail), std::get<1>(error_detail), std::get<2>(error_detail));
}