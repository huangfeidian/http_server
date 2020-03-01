
#include <iterator>
#include <regex>
#include <memory>
#include <assert.h>
#include <unordered_set>
#include <optional>
#ifdef _MSC_VER
#include <charconv>
#else
#include <exception>
#endif 
#include <http_parser.h>

namespace spiritsaway::http
{
	const int tokens[128] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0,
	};
	std::unordered_set<std::string> valid_request_method = { "GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT" };

	std::string remove_trail_blank(const std::string& input)
	{
		size_t idx = input.size();
		for (; idx > 0; idx++)
		{
			if (input[idx - 1] == ' ' || input[idx - 1] ==
				'\t')

			{
				continue;
			}
			else
			{
				break;
			}
		}
		if (idx == 0)
		{
			return std::string();
		}
		return input.substr(0, idx);
	}
	void string_to_lower_case(std::string& str)
	{
		for (auto iter = str.begin(); iter != str.end(); ++iter)
		{
			*iter = std::tolower(static_cast<char>(*iter));
		}
	}
#ifdef _MSC_VER
	template<typename T>
	std::optional<T> try_parse_unsigned_int(const std::string& _text)
	{
		T result;
		if (auto[p, ec] = std::from_chars(_text.c_str(), _text.c_str() + _text.size(), result); ec == std::errc())
		{
			return result;
		}
		return std::nullopt;

	}
#else
	template<typename T>
	std::optional<T> try_parse_unsigned_int(const std::string& text)
	{
		return std::nullopt;
	}
	template <>
	std::optional<std::uint32_t> try_parse_unsigned_int<std::uint32_t>(const std::string& text)
	{
		try
		{
			std::size_t pos = 0;
			auto result = std::stoul(text, &pos);
			if (pos != text.size())
			{
				return std::nullopt;
			}
			return result;
		}
		catch (const std::exception&)
		{
			return std::nullopt;
		}
	}
	template <>
	std::optional<std::uint64_t> try_parse_unsigned_int<std::uint64_t>(const std::string& text)
	{
		try
		{
			std::size_t pos = 0;
			auto result = std::stoull(text, &pos);
			if (pos != text.size())
			{
				return std::nullopt;
			}
			return result;
		}
		catch (const std::exception&)
		{
			return std::nullopt;
		}
	}
#endif // _MSC_VER

	
	std::tuple<reply_status, std::string, std::string> from_praser_result_to_description(http_parser_result cur_result)
	{
		switch (cur_result)
		{
		case buffer_overflow:
			return std::make_tuple(reply_status::bad_gateway, "Bad Gateway", "Response header too long");
		case invalid_method:
			return std::make_tuple(reply_status::bad_gateway, "Bad Gateway", "invalid method");
		case invalid_version:
			return std::make_tuple(reply_status::bad_gateway, "Bad Gateway", "HTTP version not supported");
		case invalid_status:
			return std::make_tuple(reply_status::bad_gateway, "Bad Gateway", "Unexpected status code");
		case invalid_transfer_encoding:
			return std::make_tuple(reply_status::bad_gateway, "Bad Gateway", "Failed to check chunked response");
		case pipeline_not_supported:
			return std::make_tuple(reply_status::bad_gateway, "Bad Gateway", "pipeline not supported");
		case bad_request:
			return std::make_tuple(reply_status::bad_gateway, "Bad Gateway", "Failed to parse response header");
		default:
			return std::make_tuple(reply_status::bad_gateway, "Bad Gateway", "unknown error " + std::to_string(cur_result));
		}
	}
	void http_request_header::reset()
	{
		_method.clear();
		_scheme.clear();
		_host.clear();
		_path_and_query.clear();
		_port = 0;
		_version = http_version::v_invalid;
		_headers_map.clear();
	}
	http_request_header::http_request_header() : _port(80)
	{
	}

	const std::string& http_request_header::method() const
	{
		return this->_method;
	}

	const std::string& http_request_header::scheme() const
	{
		return this->_scheme;
	}

	const std::string& http_request_header::host() const
	{
		return this->_host;
	}

	unsigned short http_request_header::port() const
	{
		return this->_port;
	}

	const std::string& http_request_header::path_and_query() const
	{
		return this->_path_and_query;
	}

	http_version http_request_header::get_version() const
	{
		return this->_version;
	}

	bool http_request_header::valid_method() const
	{
		if (valid_request_method.find(_method) == valid_request_method.end())
		{
			return false;
		}
		else
		{
			return true;
		}

	}
	bool http_request_header::valid_version() const
	{
		return _version != http_version::v_invalid;
	}
	bool http_request_header::is_keep_alive() const
	{
		auto connection_value = get_header_value("Connection");

		bool keep_alive = false;
		if (_version == http_version::v_1_1)
		{
			keep_alive = true;
		}
		else
		{
			keep_alive = false;
		}
		if (connection_value)
		{
			string_to_lower_case(connection_value.value());
			if (_version == http_version::v_1_1 && connection_value.value() == "close")
			{
				keep_alive = false;
			}
			else if (_version == http_version::v_1_0 && connection_value.value() == "keep-alive")
			{
				keep_alive = true;
			}
		}
		return keep_alive;
	}
	std::string http_request_header::encode_to_data() const
	{
		std::string result;
		result.reserve(500);
		result += _method;
		result += " ";
		if (_method == "CONNECT")
		{
			result += _host + ":" + std::to_string(_port);
		}
		else
		{
			result += _scheme + "://" + _host;
			if (_port != 80 && _port != 0)
			{
				result += ":" + std::to_string(_port);
			}
			result += _path_and_query;
		}
		result += " HTTP/";
		switch (_version)
		{
		case spiritsaway::http::http_version::v_invalid:
			result += "1.0";
			break;
		case spiritsaway::http::http_version::v_1_0:
			result += "1.0";
			break;
		case spiritsaway::http::http_version::v_1_1:
			result += "1.1";
			break;
		default:
			break;
		}
		result += "\r\n";
		for (const auto& header : _headers_map)
		{
			result += std::get<0>(header);
			result += ": ";
			result += std::get<1>(header);
			result += "\r\n";
		}
		result += "\r\n";
		return result;

	}
	std::optional<std::string> http_request_header::get_header_value(const std::string& name) const
	{
		auto iter = this->_headers_map.find(name);
		if (iter == this->_headers_map.end())
		{
			return std::nullopt;
		}
		return iter->second;
	}

	std::size_t http_request_header::erase_header(const std::string& name)
	{
		return this->_headers_map.erase(name);
	}

	const http_headers_container& http_request_header::get_headers_map() const
	{
		return this->_headers_map;
	}

	http_response_header::http_response_header()
	{
	}
	void http_response_header::reset()
	{
		_version = http_version::v_invalid;
		_status_code = reply_status::ok;
		_status_description.clear();
		_headers_map.clear();

	}
	bool http_response_header::set_version(const std::string& _in_version)
	{
		if (_in_version == "1.0")
		{
			_version = http_version::v_1_0;
			return true;
		}
		else if (_in_version == "1.1")
		{
			_version = http_version::v_1_1;
			return true;
		}
		return false;
	}


	void http_response_header::status_code(reply_status in_status_code)
	{
		_status_code = in_status_code;
	}

	const std::string& http_response_header::status_description() const
	{
		return this->_status_description;
	}
	void http_response_header::status_description(const std::string& _desc)
	{
		_status_description = _desc;
	}

	std::optional<std::string> http_response_header::get_header_value(const std::string& name) const
	{
		auto iter = this->_headers_map.find(name);
		if (iter == this->_headers_map.end())
		{
			return std::nullopt;
		}
		return iter->second;
	}
	void http_response_header::add_header_value(const std::string& key, const std::string& value)
	{
		_headers_map.insert(std::make_pair(key, value));
	}

	std::size_t http_response_header::erase_header(const std::string& name)
	{
		return this->_headers_map.erase(name);
	}

	const http_headers_container& http_response_header::get_headers_map() const
	{
		return this->_headers_map;
	}


	std::string http_response_header::encode_to_data() const
	{
		std::string result;
		result.reserve(500);
		result += "HTTP/";
		switch (_version)
		{
		case spiritsaway::http::http_version::v_invalid:
			result += "1.0";
			break;
		case spiritsaway::http::http_version::v_1_0:
			result += "1.0";
			break;
		case spiritsaway::http::http_version::v_1_1:
			result += "1.1";
			break;
		default:
			break;
		}
		result.push_back(' ');
		result += std::to_string(static_cast<int>(_status_code));
		if (!_status_description.empty())
		{
			result.push_back(' ');
			result += _status_description;
		}
		result += "\r\n";

		for (const auto& header : _headers_map)
		{
			result += std::get<0>(header);
			result += ": ";
			result += std::get<1>(header);
			result += "\r\n";
		}
		result += "\r\n";
		return result;
	}
	std::string http_response_header::encode_to_data(const std::string& content)
	{
		auto cur_content_size = content.size();
		_headers_map.erase("Content-Length");
		_headers_map.insert(std::make_pair(std::string("Content-Length"), std::to_string(cur_content_size)));
		auto header_result = encode_to_data();
		header_result += content;
		return header_result;
	}

	std::pair<http_parser_result, std::uint32_t> http_header_parser::parse_headers(const unsigned char* begin, const unsigned char* end, http_headers_container& headers)
	{
		//lambdas are inlined  don't worry be happy
		auto is_token_char = [](char ch) -> bool
		{
			if (ch >= 128)
			{
				return false;
			}
			return tokens[ch];
		};

		enum class parse_header_state
		{
			header_field_name_start,
			header_field_name,
			header_field_value_left_ows,
			header_field_value,
			header_field_cr,
			header_field_crlf,
			header_field_crlfcr,
			header_compelete,
			header_parse_failed
		};

		parse_header_state state = parse_header_state::header_field_name_start;
		std::string header_field_name;
		std::string header_field_value;
		//哈哈一个状态机，不过模拟的不彻底啊，话说这位同学为什么这么喜欢字节流的状态机呢 
		//应该是网络流并不是一次传输完成的 只有字节流的边界是清晰的
		auto iter = begin;
		for (; iter != end && state != parse_header_state::header_compelete && state != parse_header_state::header_parse_failed; ++iter)
		{
			switch (state)
			{
			case parse_header_state::header_field_name_start:
				if (is_token_char(*iter))
				{
					header_field_name.push_back(*iter);
					state = parse_header_state::header_field_name;
				}
				else
				{
					if (iter == begin && *iter == '\r')
					{
						state = parse_header_state::header_field_crlfcr;
					}
					else
					{
						state = parse_header_state::header_parse_failed;
					}
				}
				break;
			case parse_header_state::header_field_name:
				if (is_token_char(*iter))
				{
					header_field_name.push_back(*iter);
				}
				else
				{
					if (*iter == ':')
					{
						state = parse_header_state::header_field_value_left_ows;
					}
					else
					{
						state = parse_header_state::header_parse_failed;
					}
				}
				break;
			case parse_header_state::header_field_value_left_ows:
				if (*iter == ' ' || *iter == '\t')
				{
					continue;
				}
				else
				{
					if (*iter == '\r')
					{
						state = parse_header_state::header_field_cr;
					}
					else
					{
						header_field_value.push_back(*iter);
						state = parse_header_state::header_field_value;
					}
				}
				break;
			case parse_header_state::header_field_value:
				if (*iter == '\r')
				{
					state = parse_header_state::header_field_cr;
				}
				else
				{
					header_field_value.push_back(*iter);
				}
				break;
			case parse_header_state::header_field_cr:
				if (*iter == '\n')
				{
					state = parse_header_state::header_field_crlf;
				}
				else
				{
					state = parse_header_state::header_parse_failed;
				}
				break;
			case parse_header_state::header_field_crlf:
				if (*iter == ' ' || *iter == '\t')
				{
					header_field_value.push_back(*iter);
					state = parse_header_state::header_field_value;
				}
				else
				{
					//删掉末尾的空白字符
					headers.insert(std::make_pair(header_field_name, remove_trail_blank(header_field_value)));
					header_field_name.clear();
					header_field_value.clear();
					if (*iter == '\r')
					{
						state = parse_header_state::header_field_crlfcr;
					}
					else
					{
						if (is_token_char(*iter))
						{
							header_field_name.push_back(*iter);
							state = parse_header_state::header_field_name;
						}
						else
						{
							state = parse_header_state::header_parse_failed;
						}
					}
				}
				break;
			case parse_header_state::header_field_crlfcr:
				if (*iter == '\n')
				{
					state = parse_header_state::header_compelete;
				}
				break;
			default:
				assert(false);
			}
		}
		if (state != parse_header_state::header_compelete)
		{
			return std::make_pair(http_parser_result::buffer_overflow, 0);
		}
		return std::make_pair(http_parser_result::read_one_header, iter - begin);
	}

	bool http_header_parser::parse_uri(const std::string& uri, http_request_header& header)
	{
		if (uri.find("://") != std::string::npos)
		{
			//GET http://download.microtool.de:80/somedata.exe
			std::regex regex("(.+?)://(.+?)(:(\\d+))?(/.*)"); 
			std::match_results<std::string::const_iterator> match_results;
			if (!std::regex_match(uri.begin(), uri.end(), match_results, regex))
			{
				return http_parser_result::invalid_uri;
			}
			header._scheme = match_results[1];
			header._host = match_results[2];
			if (match_results[4].matched)
			{
				auto port_result = try_parse_unsigned_int<std::uint32_t>(std::string(match_results[4]));
				if (!port_result)
				{
					return false;
				}
				if (port_result.value() >= std::numeric_limits<std::uint16_t>::max())
				{
					return false;
				}
				header._port = port_result.value();
			}
			header._path_and_query = match_results[5];
			return true;
		}
		else
		{
			// 127.0.0.1:8090
			std::regex regex("(.+?)(:(\\d+))?(/.*)?"); //GET http://download.microtool.de:80/somedata.exe
			std::match_results<std::string::const_iterator> match_results;
			if (!std::regex_match(uri.begin(), uri.end(), match_results, regex))
			{
				return false;
			}
			header._host = match_results[1];
			if (match_results[3].matched)
			{
				auto port_result = try_parse_unsigned_int<std::uint32_t>(std::string(match_results[3]));
				if (!port_result)
				{
					return false;
				}
				if (port_result.value() >= std::numeric_limits<std::uint16_t>::max())
				{
					return false;
				}
				header._port = port_result.value();
			}
			if (match_results[4].matched)
			{
				header._path_and_query = match_results[4];
			}
			return true;
		}
		
	}
	
	http_parser_result http_header_parser::parse_request_header(const unsigned char* begin, const unsigned char* end, http_request_header& header)
	{

		auto iter = begin;
		auto tmp = iter;
		for (; iter != end && *iter != ' ' && *iter != '\r'; ++iter)
		{
			//get the method
		}
		if (iter == tmp || iter == end || *iter != ' ')
		{
			return http_parser_result::buffer_overflow;
		}
		header._method = std::string(tmp, iter);
		tmp = ++iter;
		for (; iter != end && *iter != ' ' && *iter != '\r'; ++iter)
		{
			//get  the url
		}
		if (iter == tmp || iter == end || *iter != ' ')
		{
			return http_parser_result::buffer_overflow;
		}
		auto request_uri = std::string(tmp, iter);
		if (header.method() == "CONNECT")
		{
			std::regex regex("(.+?):(\\d+)"); //host:port
			std::match_results<std::string::iterator> match_results;
			if (!std::regex_match(request_uri.begin(), request_uri.end(), match_results, regex))
			{
				return http_parser_result::buffer_overflow;
			}
			header._host = match_results[1];
			auto port_result = try_parse_unsigned_int<std::uint32_t>(std::string(match_results[2]));
			if (!port_result)
			{
				return http_parser_result::buffer_overflow;
			}
			if (port_result.value() >= std::numeric_limits<std::uint16_t>::max())
			{
				return http_parser_result::buffer_overflow;
			}
			header._port = port_result.value();
		}
		else
		{
			if (request_uri.find("://") != std::string::npos)
			{
				if (!parse_uri(request_uri, header))
				{
					return http_parser_result::invalid_uri;
				}
			}
			else
			{
				header._scheme = "http";
				header._path_and_query = request_uri;
				header._port = 80;
			}
			
		}

		tmp = ++iter;
		for (; iter != end && *iter != '\r'; ++iter)
		{
			//to the end of line 
		}
		// HTTP/x.y
		if (iter == end || std::distance(tmp, iter) < 6 || !std::equal(tmp, tmp + 5, "HTTP/"))
		{
			return http_parser_result::buffer_overflow;
		}
		auto cur_version_str = std::string(tmp + 5, iter);
		if (cur_version_str == "1.0")
		{
			header._version = http_version::v_1_0;
		}
		else if (cur_version_str == "1.1")
		{
			header._version = http_version::v_1_1;
		}
		else
		{
			return http_parser_result::invalid_version;
		}

		++iter;
		if (iter == end || *iter != '\n')
		{
			return http_parser_result::buffer_overflow;
		}

		++iter;
		auto parse_header_result = parse_headers(reinterpret_cast<const unsigned char*>(&(*iter)), reinterpret_cast<const unsigned char*>(&(*end)), header._headers_map);
		if (parse_header_result.first >= http_parser_result::parse_error)
		{
			return parse_header_result.first;
		}
		auto cur_host_value = header.get_header_value("Host");
		if (cur_host_value)
		{
			auto cur_host_str = cur_host_value.value();
			if (!parse_uri(cur_host_str, header))
			{
				return http_parser_result::invalid_uri;
			}

		}
		return http_parser_result::read_one_header;
	}

	http_parser_result http_header_parser::parse_response_header(const unsigned char* begin, const unsigned char* end, http_response_header& header)
	{
		auto iter = begin;
		auto tmp = iter;
		for (; iter != end && *iter != ' ' && *iter != '\r'; ++iter)
		{
			// to the end of line
		}
		if (std::distance(tmp, iter) < 6 || iter == end || *iter != ' ' || !std::equal(tmp, tmp + 5, "HTTP/")) return http_parser_result::buffer_overflow;
		auto cur_version_str = std::string(tmp + 5, iter);
		if (cur_version_str == "1.0")
		{
			header._version = http_version::v_1_0;
		}
		else if (cur_version_str == "1.1")
		{
			header._version = http_version::v_1_1;
		}
		else
		{
			return http_parser_result::invalid_version;
		}
		tmp = ++iter;
		for (; iter != end && *iter != ' ' && *iter != '\r'; ++iter)
		{

		}
		if (tmp == iter || iter == end)
		{
			return http_parser_result::buffer_overflow;
		}

		auto status_result = try_parse_unsigned_int<std::uint32_t>(std::string(std::string(tmp, iter)));
		if (!status_result)
		{
			return http_parser_result::buffer_overflow;
		}
		if (status_result.value() >= std::numeric_limits<std::uint16_t>::max())
		{
			return http_parser_result::buffer_overflow;
		}
		header._status_code = static_cast<reply_status>(status_result.value());

		if (*iter == ' ')
		{
			tmp = ++iter;
			for (; iter != end && *iter != '\r'; ++iter)
			{

			}
			if (iter == end || *iter != '\r')
			{
				return http_parser_result::buffer_overflow;
			}
			header._status_description = std::string(tmp, iter);
		}

		if (*iter != '\r')
		{
			return http_parser_result::buffer_overflow;
		}

		if (iter == end || *(++iter) != '\n')
		{
			return http_parser_result::buffer_overflow;
		}

		++iter;

		auto parse_header_result = parse_headers(reinterpret_cast<const unsigned char*>(&(*iter)), reinterpret_cast<const unsigned char*>(&(*end)), header._headers_map);
		if (parse_header_result.first >= http_parser_result::parse_error)
		{
			return http_parser_result::parse_error;
		}


		return http_parser_result::read_one_header;
	}
	http_request_parser::http_request_parser(bool pipeline_allowed):
		_pipeline_allowed(pipeline_allowed)
	{
		buffer_size = 0;
		parser_idx = 0;
		_status = http_parser_status::read_header;
	}
	bool http_request_parser::append_input(const unsigned char* in_bytes, std::size_t length)
	{
		if (parser_idx >= MAX_HTTP_BUFFER_LENGTH / 2)
		{
			
			std::copy(buffer + parser_idx, buffer + buffer_size, buffer);
			buffer_size = buffer_size - parser_idx;
			parser_idx = 0;
		}
	
		if (length + buffer_size >= MAX_HTTP_BUFFER_LENGTH)
		{
			return false;
		}
		std::copy(in_bytes, in_bytes + length, buffer + buffer_size);
		buffer_size = buffer_size + length;
		return true;
	}
	std::pair<http_parser_result, std::string_view> http_request_parser::parse()
	{
		std::uint32_t double_crlf_pos = buffer_size;
		if (_status == http_parser_status::read_header_only_finish)
		{
			_status = http_parser_status::read_header;
			return std::make_pair(http_parser_result::read_content_end, std::string_view());
		}
		if (parser_idx == buffer_size)
		{
			return std::make_pair(http_parser_result::waiting_input, std::string_view());
		}
		if (_status == http_parser_status::read_header)
		{
			if (buffer_size - parser_idx < 4)
			{
				return std::make_pair(http_parser_result::reading_header, std::string_view());
			}
			for (std::uint32_t i = parser_idx; i <= buffer_size - 4; i++)
			{
				if (buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n')
				{
					double_crlf_pos = i;
					break;
				}
			}
			if (double_crlf_pos == buffer_size)
			{
				return std::make_pair(http_parser_result::reading_header, std::string_view());
			}
			
			auto cur_parse_result = http_header_parser::parse_request_header(reinterpret_cast<const unsigned char*>(buffer) + parser_idx, reinterpret_cast<const unsigned char*>(buffer) + double_crlf_pos + 4, _header);
			parser_idx = double_crlf_pos + 4;
			if (cur_parse_result != http_parser_result::read_one_header)
			{
				return std::make_pair(http_parser_result::bad_request, std::string_view());
			}
			if (!_header.valid_method())
			{
				return std::make_pair(http_parser_result::invalid_method, std::string_view());
			}
			if (!_header.valid_version())
			{
				return std::make_pair(http_parser_result::invalid_version, std::string_view());
			}
			total_content_length = 0;
			read_content_length = 0;
			_cur_chunk_checker.reset();
			const auto& cur_method = _header.method();
			if (cur_method == "GET" || cur_method == "HEAD" || cur_method == "DELETE" ||cur_method == "CONNECT")
			{
				_status = http_parser_status::read_header_only_finish;
				return std::make_pair(http_parser_result::read_one_header, std::string_view());
			}
			else if (cur_method == "POST" || cur_method == "PUT")
			{
				auto content_length_value = _header.get_header_value("Content-Length");
				auto transfer_encoding_value = _header.get_header_value("Transfer-Encoding");
				if (content_length_value)
				{
					auto length_opt = try_parse_unsigned_int<std::uint64_t>(content_length_value.value());
					if (!length_opt)
					{
						return std::make_pair(http_parser_result::bad_request, std::string_view());
					}
					total_content_length = length_opt.value();
					_status = http_parser_status::read_content;
				}
				else if (transfer_encoding_value)
				{
					string_to_lower_case(transfer_encoding_value.value());
					if (transfer_encoding_value.value() == "chunked")
					{
						_cur_chunk_checker.reset();
						_status = http_parser_status::read_chunked;
					}
					else
					{
						return std::make_pair(http_parser_result::invalid_transfer_encoding, std::string_view());
					}
				}
				else
				{
					return std::make_pair(http_parser_result::parse_error, std::string_view());
				}
			}
			else
			{
				return std::make_pair(http_parser_result::parse_error, std::string_view());
			}
			return std::make_pair(http_parser_result::read_one_header, std::string_view());
		}
		else if (_status == http_parser_status::read_content)
		{
			if ((buffer_size - parser_idx) >= (total_content_length - read_content_length))
			{
				_status = http_parser_status::read_header;
				auto cur_content_length = static_cast<std::size_t>(total_content_length - read_content_length);
				auto cur_result_buffer = std::string_view(buffer + parser_idx, cur_content_length);
				parser_idx += cur_content_length;
				return std::make_pair(http_parser_result::read_content_end, cur_result_buffer);
			}
			else
			{
				auto cur_result_buffer = std::string_view(buffer + parser_idx, buffer_size - parser_idx);
				parser_idx = buffer_size;
				return std::make_pair(http_parser_result::read_some_content, cur_result_buffer);
			}
		}
		else if (_status == http_parser_status::read_chunked)
		{
			auto chunk_parse_result = _cur_chunk_checker.check(buffer + parser_idx, buffer + buffer_size);
			if (!chunk_parse_result.first)
			{
				return std::make_pair(http_parser_result::chunk_check_error, std::string_view());
			}
			auto cur_result_bufer = std::string_view(buffer + parser_idx, chunk_parse_result.second);
			parser_idx += chunk_parse_result.second;
			if (_cur_chunk_checker.is_complete())
			{
				_status = http_parser_status::read_header;
				return std::make_pair(http_parser_result::read_content_end, cur_result_bufer);
			}
			else
			{
				return std::make_pair(http_parser_result::read_some_content, cur_result_bufer);
			}
			
		}
		else
		{
			return std::make_pair(http_parser_result::invalid_parser_status, std::string_view());
		}
	}

	http_response_parser::http_response_parser(bool pipeline_allowed):
		_pipeline_allowed(pipeline_allowed)
	{
		buffer_size = 0;
		parser_idx = 0;
		_status = http_parser_status::read_header;
	}
	bool http_response_parser::append_input(const unsigned char* in_bytes, std::size_t length)
	{
		if (parser_idx >= MAX_HTTP_BUFFER_LENGTH / 2)
		{

			std::copy(buffer + parser_idx, buffer + buffer_size, buffer);
			buffer_size = buffer_size - parser_idx;
			parser_idx = 0;
		}

		if (length + buffer_size >= MAX_HTTP_BUFFER_LENGTH)
		{
			return false;
		}
		std::copy(in_bytes, in_bytes + length, buffer + buffer_size);
		buffer_size = buffer_size + length;
		return true;
	}
	std::pair<http_parser_result, std::string_view> http_response_parser::parse()
	{
		std::uint32_t double_crlf_pos = buffer_size;
		if (parser_idx == buffer_size)
		{
			return std::make_pair(http_parser_result::waiting_input, std::string_view());
		}
		if (_status == http_parser_status::read_header)
		{
			if (buffer_size - parser_idx < 4)
			{
				return std::make_pair(http_parser_result::reading_header, std::string_view());
			}
			for (std::uint32_t i = parser_idx; i <= buffer_size - 4; i++)
			{
				if (buffer[i] == '\r' && buffer[i + 1] == '\n' && buffer[i + 2] == '\r' && buffer[i + 3] == '\n')
				{
					double_crlf_pos = i;
					break;
				}
			}
			if (double_crlf_pos == buffer_size)
			{
				return std::make_pair(http_parser_result::reading_header, std::string_view());
			}
			
			auto cur_parse_result = http_header_parser::parse_response_header(reinterpret_cast<const unsigned char*>(buffer) + parser_idx, reinterpret_cast<const unsigned char*>(buffer) + double_crlf_pos + 4, _header);
			parser_idx = double_crlf_pos + 4;
			if (cur_parse_result != http_parser_result::read_one_header)
			{
				return std::make_pair(http_parser_result::bad_request, std::string_view());
			}

			if (_header._version == http_version::v_invalid)
			{
				return std::make_pair(http_parser_result::invalid_version, std::string_view());
			}
			total_content_length = 0;
			read_content_length = 0;
			_cur_chunk_checker.reset();

			auto content_length_value = _header.get_header_value("Content-Length");
			auto transfer_encoding_value = _header.get_header_value("Transfer-Encoding");
			if (content_length_value)
			{
				auto length_opt = try_parse_unsigned_int<std::uint64_t>(content_length_value.value());
				if (!length_opt)
				{
					return std::make_pair(http_parser_result::bad_request, std::string_view());
				}
				total_content_length = length_opt.value();
				if (total_content_length > 0)
				{
					_status = http_parser_status::read_content;
				}
				else
				{
					_status = http_parser_status::read_header;
					if(parser_idx != buffer_size && !_pipeline_allowed)
					{ 
						return std::make_pair(http_parser_result::pipeline_not_supported, std::string_view());
					}
				}
				
			}
			else if (transfer_encoding_value)
			{
				string_to_lower_case(transfer_encoding_value.value());
				if (transfer_encoding_value.value() == "chunked")
				{
					_cur_chunk_checker.reset();
					_status = http_parser_status::read_chunked;
				}
				else
				{
					return std::make_pair(http_parser_result::invalid_transfer_encoding, std::string_view());
				}
			}
			else
			{
				_status = http_parser_status::read_header;
				if (parser_idx != buffer_size && !_pipeline_allowed)
				{
					return std::make_pair(http_parser_result::pipeline_not_supported, std::string_view());
				}
			}
			return std::make_pair(http_parser_result::read_one_header, std::string_view());
		}
		else if (_status == http_parser_status::read_content)
		{
			if ((buffer_size - parser_idx) >= (total_content_length - read_content_length))
			{
				if ((buffer_size - parser_idx > total_content_length - read_content_length) && !_pipeline_allowed)
				{
					return std::make_pair(http_parser_result::pipeline_not_supported, std::string_view());
				}
				_status = http_parser_status::read_header;
				auto cur_content_length = static_cast<std::size_t>(total_content_length - read_content_length);
				auto cur_result_buffer = std::string_view(buffer + parser_idx, cur_content_length);
				parser_idx += cur_content_length;
				return std::make_pair(http_parser_result::read_content_end, cur_result_buffer);
			}
			else
			{
				auto cur_result_buffer = std::string_view(buffer + parser_idx, buffer_size - parser_idx);
				parser_idx = buffer_size;
				return std::make_pair(http_parser_result::read_some_content, cur_result_buffer);
			}
		}
		else if (_status == http_parser_status::read_chunked)
		{
			auto chunk_parse_result = _cur_chunk_checker.check(buffer + parser_idx, buffer + buffer_size);
			if (!chunk_parse_result.first)
			{
				return std::make_pair(http_parser_result::chunk_check_error, std::string_view());
			}
			auto cur_result_buferr = std::string_view(buffer + parser_idx, chunk_parse_result.second);
			parser_idx += chunk_parse_result.second;
			if (_cur_chunk_checker.is_complete())
			{
				_status = http_parser_status::read_header;
				if (parser_idx != buffer_size && !_pipeline_allowed)
				{
					return std::make_pair(http_parser_result::pipeline_not_supported, std::string_view());
				}
				return std::make_pair(http_parser_result::read_content_end, cur_result_buferr);

			}
			else
			{
				return std::make_pair(http_parser_result::read_some_content, cur_result_buferr);
			}
			
		}
		else
		{
			return std::make_pair(http_parser_result::invalid_parser_status, std::string_view());
		}
	}
	std::optional<bool> http_response_header::is_keep_alive() const
	{
		auto connection_value = get_header_value("Connection");
		auto result = false;
		if (_version == http_version::v_1_1)
		{
			result = true;
		}
		else
		{
			result = false;
		}
		if (connection_value)
		{
			string_to_lower_case(connection_value.value());
			if (connection_value.value() == "close")
			{
				result = false;
			}
			else if (connection_value.value() == "keep-alive")
			{
				result = true;
			}
			else
			{
				return std::nullopt;
			}
		}
		return result;
	}
	void http_response_parser::reset_header()
	{
		_header.reset();
	}
	void http_request_parser::reset_header()
	{
		_header.reset();
	}
	http_parser_status http_response_parser::status() const
	{
		// 判断目前是否刚好读取完一个packet
		return _status;
	}
	http_parser_status http_request_parser::status() const
	{
		// 判断目前是否刚好读取完一个packet
		return _status;
	}
	void http_request_parser::reset()
	{
		_status = http_parser_status::read_header;
		parser_idx = 0;
		buffer_size = 0;
		_header.reset();
		_cur_chunk_checker.reset();
		total_content_length = 0;
		read_content_length = 0;
	}
	void http_response_parser::reset()
	{
		_status = http_parser_status::read_header;
		parser_idx = 0;
		buffer_size = 0;
		_header.reset();
		_cur_chunk_checker.reset();
		total_content_length = 0;
		read_content_length = 0;
	}
}; // namespace http_proxy
