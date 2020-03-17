#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <string_view>
#include <optional>

#include "http_chunk_parser.h"
#include "http_const.h"


namespace spiritsaway::http
{
	const std::size_t MAX_HTTP_BUFFER_LENGTH = 4096 * 16;
	const std::size_t BUFFER_LENGTH = 2048 * 16 ;
	
	enum http_parser_status: uint16_t
	{
		read_header,
		read_content,
		read_chunked,
		read_header_only_finish,

	};
	enum http_parser_result
	{
		reading_header,
		read_one_header,
		reading_content,
		reading_chunk,
		read_some_content,
		read_content_end,
		waiting_input,
		parse_error,
		pipeline_not_supported,
		buffer_overflow,
		bad_request,
		invalid_method,
		invalid_version,
		invalid_status,
		invalid_uri,
		invalid_transfer_encoding,
		chunk_check_error,
		invalid_parser_status,

	};
	std::tuple<reply_status, std::string, std::string> from_praser_result_to_description(http_parser_result cur_result);
	struct default_filed_name_compare
	{
		bool operator() (const std::string& str1, const std::string& str2) const
		{
			return std::lexicographical_compare(str1.begin(), str1.end(), str2.begin(), str2.end(), [](const char ch1, const char ch2) -> bool
			{
				return std::tolower(static_cast<char>(ch1)) < std::tolower(static_cast<char>(ch2));
			});
		}
	};

	// https://stackoverflow.com/questions/4371328/are-duplicate-http-response-headersacceptable
	//Cache-Control: no-cache
	//Cache-Control: no - store
	// 因为http头里面会有重复的key 所以这里只能用multimap
	typedef std::multimap<const std::string, std::string, default_filed_name_compare> http_headers_container;

	class http_request_header
	{
		friend class http_header_parser;
		friend class http_parser;
		friend class http_request_parser;
		std::string _method;
		std::string _scheme;
		std::string _host;
		unsigned short _port;
		std::string _path_and_query;
		http_version _version;
		http_headers_container _headers_map;
		
		void reset();
	public:
		http_request_header();
		const std::string& method() const;
		const std::string& scheme() const;
		const std::string& host() const;
		unsigned short port() const;
		const std::string& path_and_query() const;
		http_version get_version() const;
		std::optional<std::string> get_header_value(const std::string& name) const;
		std::size_t erase_header(const std::string& name);
		const http_headers_container& get_headers_map() const;
		bool valid_method() const;
		bool valid_version() const;
		std::string encode_to_data() const;
		bool is_keep_alive() const;

	};

	class http_response_header
	{
		friend class http_response_parser;
		friend class http_parser;
		friend class http_header_parser;
		http_version _version;
		reply_status _status_code;
		std::string _status_description;
		http_headers_container _headers_map;
		void reset();
		
	public:
		http_response_header();
		bool set_version(const std::string& _version);
		void status_code(reply_status _status_code);
		void status_description(const std::string& _status_desc);
		void add_header_value(const std::string& key, const std::string& value);
		const std::string& status_description() const;
		std::optional<std::string> get_header_value(const std::string& name) const;
		std::size_t erase_header(const std::string& name);
		const http_headers_container& get_headers_map() const;
		std::string encode_to_data() const;
		std::string encode_to_data(const std::string& content) ;
		std::optional<bool> is_keep_alive() const;
		
	};

	class http_header_parser
	{
		static std::pair<http_parser_result, std::uint32_t> parse_headers(const unsigned char* begin, const unsigned char* end, http_headers_container& headers);
	public:
		static http_parser_result parse_request_header(const unsigned char* begin, const unsigned char* end, http_request_header& header);
		static http_parser_result parse_response_header(const unsigned char* begin, const unsigned char* end, http_response_header& header);
		static bool parse_uri(const std::string& uri, http_request_header& header);
	};
	void string_to_lower_case(std::string& str);
	std::string remove_trail_blank(const std::string& input);

	class http_request_parser
	{
	private:
		char buffer[MAX_HTTP_BUFFER_LENGTH];
		std::uint32_t buffer_size;
		std::uint32_t parser_idx;
		std::uint64_t total_content_length;
		std::uint64_t read_content_length;
		http_chunk_parser _cur_chunk_checker;
		bool _pipeline_allowed;
		http_parser_status _status;
	public:
		http_request_header _header;
		
		std::pair<http_parser_result, std::string_view> parse();
		bool append_input(const unsigned char* in_bytes, std::size_t length);
		void reset_header();
		http_request_parser(bool pipeline_allowed = false);
		http_parser_status status() const;
		void reset();
	};

	class http_response_parser
	{
	private:
		char buffer[MAX_HTTP_BUFFER_LENGTH];
		std::uint32_t buffer_size;
		std::uint32_t parser_idx;
		std::uint64_t total_content_length;
		std::uint64_t read_content_length;
		http_chunk_parser _cur_chunk_checker;
		bool _pipeline_allowed;
		http_parser_status _status;
	public:
		http_response_header _header;
		
		std::pair<http_parser_result, std::string_view> parse();
		bool append_input(const unsigned char* in_bytes, std::size_t length);
		void reset_header();
		http_response_parser(bool pipeline_allowed = false);
		http_parser_status status() const;
		void reset();
	};
}; // namespace http_proxy
