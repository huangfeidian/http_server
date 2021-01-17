
#include <tuple>
#include "http_parser.h"
#include "http_packet.hpp"

namespace spiritsaway::http_server
{
	/// Parser for incoming requests.
	class request_parser
	{
	public:
		/// Construct ready to parse the request method.
		request_parser();

		void move_req(request &dest);

		/// Result of parse.
		enum class result_type
		{
			good,
			bad,
			indeterminate
		};

		/// Parse some data. The enum return value is good when a complete request has
		/// been parsed, bad if the data is invalid, indeterminate when more data is
		/// required. The InputIterator return value indicates how much of the input
		/// has been consumed.
		///
		result_type parse(const char *input, std::size_t len);

	private:
	public:
		request req_;
		bool req_complete_ = false;

	private:
		http_parser_settings parse_settings_;
		http_parser parser_;
	};

} // namespace spiritsaway::http_server
