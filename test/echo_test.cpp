#include <http_connection.h>
#include <http_server.h>

using namespace spiritsaway::http;
using namespace std;

class echo_connection:public http_connection
{
public:
	static std::shared_ptr<echo_connection> create(asio::ip::tcp::socket&& _in_client_socket, std::shared_ptr<spdlog::logger> logger, std::uint32_t in_connection_idx, std::uint32_t _in_timeout, std::string log_pre, void* common_data)
		
	{
		return std::make_shared<echo_connection>( std::move(_in_client_socket), logger, in_connection_idx, _in_timeout, log_pre);
	}
	echo_connection( asio::ip::tcp::socket&& in_client_socket, std::shared_ptr<spdlog::logger> logger, std::uint32_t in_connection_idx, std::uint32_t in_timeout, std::string log_pre = "connection")
		: http_connection(std::move(in_client_socket), logger, in_connection_idx, in_timeout, log_pre)
	{

	}
	void on_client_data_body_read(const http_request_header& _header, std::string_view _content)
	{
		http_response_header _cur_response;
		_cur_response.set_version("1.0");
		_cur_response.status_code(reply_status::ok);
		_cur_response.status_description("OK");

		_cur_response.add_header_value("Content-Type", "text/html");
		_cur_response.add_header_value("Server", "Http Server");
		auto request_str = _header.encode_to_data();
		response_str = _cur_response.encode_to_data(request_str);
		
	}
};
int main()
{
	asio::io_context cur_context;
	http_server<echo_connection> cur_server(cur_context, "echo_server", 8090);
	cur_server.run();
	cur_context.run();
}