#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
namespace spiritsaway::http_server
{
    struct header
	{
		std::string name;
		std::string value;
	};
    /// A request received from a client.
    struct request
    {
        std::string method;
        std::string uri;
        int http_version_major;
        int http_version_minor;
        std::vector<header> headers;
        std::string body;
    };

    /// A reply to be sent to a client.
    struct reply
    {
        /// The status of the reply.
        enum class status_type
        {
            ok = 200,
            created = 201,
            accepted = 202,
            no_content = 204,
            multiple_choices = 300,
            moved_permanently = 301,
            moved_temporarily = 302,
            not_modified = 304,
            bad_request = 400,
            unauthorized = 401,
            forbidden = 403,
            not_found = 404,
            internal_server_error = 500,
            not_implemented = 501,
            bad_gateway = 502,
            service_unavailable = 503
        } status;

        /// The headers to be included in the reply.
        std::vector<header> headers;

        /// The content to be sent in the reply.
        std::string content;


        std::string to_string();

        /// Get a stock reply.
        static reply stock_reply(status_type status);
    };
    using reply_handler = std::function<void(const reply& rep)>;
    using request_handler = std::function<void(std::weak_ptr<request> req, reply_handler cb)>;
}