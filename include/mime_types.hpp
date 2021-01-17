#pragma once

#include <string>

namespace spiritsaway::http_server::mime_types{
/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);
}

