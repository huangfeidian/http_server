#include <http_server.hpp>
#include <iostream>
using namespace spiritsaway::http_server;
using namespace std;



int main()
{
	asio::io_context cur_context;

	try
	{
		// Check command line arguments.
		//if (argc != 2)
		//{
		//  std::cerr << "Usage: http_server <port>\n";
		//  return 1;
		//}

		// Initialise the server.
		request_handler echo_handler_ins = [](std::weak_ptr< request> weak_req, reply_handler cb)
		{
			auto req_ptr = weak_req.lock();
			if(!req_ptr)
			{
				return;
			}
			auto& req = *req_ptr;
			reply rep;
			// Fill out the reply to be sent to the client.
			
			rep.content = "echo request uri: " + req.uri + " body: " + req.body;
			rep.stock_reply(reply::status_type::ok);
			
			cb(rep);
		};
		std::string address = "127.0.0.1";
		std::string port = "8080";
		server s(cur_context, address, port, echo_handler_ins);

		// Run the server until stopped.
		s.run();
		cur_context.run();
	}
	catch (std::exception &e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}

	return 0;
}