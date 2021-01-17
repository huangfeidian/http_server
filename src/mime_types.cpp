

#include "mime_types.hpp"

namespace spiritsaway::http_server::mime_types
{

	struct mapping
	{
		const char *extension;
		const char *mime_type;
	} mappings[] =
		{
			{"gif", "image/gif"},
			{"htm", "text/html"},
			{"html", "text/html"},
			{"jpg", "image/jpeg"},
			{"png", "image/png"}};

	std::string extension_to_type(const std::string &extension)
	{
		for (mapping m : mappings)
		{
			if (m.extension == extension)
			{
				return m.mime_type;
			}
		}

		return "text/plain";
	}

} // namespace spiritsaway::http_server::mime_types
