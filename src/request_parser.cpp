#include "request_parser.hpp"

namespace spiritsaway::http_server
{
    namespace
    {
        int on_url_cb(http_parser *parser, const char *at, std::size_t length)
        {
            auto &t = *reinterpret_cast<request_parser *>(parser->data);
            t.req_.uri.append(at, length);
            return 0;
        }
        int on_body_cb(http_parser *parser, const char *at, std::size_t length)
        {
            auto &t = *reinterpret_cast<request_parser *>(parser->data);

            t.req_.body.append(at, length);
            return 0;
        }
        int on_header_field_cb(http_parser *parser, const char *at, std::size_t length)
        {
            auto &t = *reinterpret_cast<request_parser *>(parser->data);
            header temp_header;
            temp_header.name = std::string(at, length);
            t.req_.headers.push_back(temp_header);
            return 0;
        }
        int on_header_value_cb(http_parser *parser, const char *at, std::size_t length)
        {
            auto &t = *reinterpret_cast<request_parser *>(parser->data);

            t.req_.headers.back().value = std::string(at, length);
            return 0;
        }
        int on_header_complete_cb(http_parser *parser)
        {
            return 0;
        }
        int on_message_complete_cb(http_parser *parser)
        {
            auto &t = *reinterpret_cast<request_parser *>(parser->data);
            t.req_complete_ = true;
            return 0;
        }
    } // namespace
    request_parser::request_parser()
        : parser_(), parse_settings_()
    {
        http_parser_init(&parser_, http_parser_type::HTTP_REQUEST);
        parser_.data = reinterpret_cast<void *>(this);
        parse_settings_.on_url = on_url_cb;
        parse_settings_.on_body = on_body_cb;
        parse_settings_.on_header_field = on_header_field_cb;
        parse_settings_.on_header_value = on_header_value_cb;
        parse_settings_.on_headers_complete = on_header_complete_cb;
        parse_settings_.on_message_complete = on_message_complete_cb;
    }
    request_parser::result_type request_parser::parse(const char *input, std::size_t len)
    {
        std::size_t nparsed = http_parser_execute(&parser_, &parse_settings_, input, len);
        if (parser_.upgrade)
        {
            return request_parser::result_type::bad;
        }
        if (nparsed != len)
        {
            return request_parser::result_type::bad;
        }
        if (req_complete_)
        {
            return request_parser::result_type::good;
        }
        else
        {
            return request_parser::result_type::indeterminate;
        }
    }
    void request_parser::move_req(request &dest)
    {
        dest = std::move(req_);
    }

} // namespace spiritsaway::http_server
