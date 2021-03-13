

#include "connection_manager.hpp"

namespace spiritsaway::http_server
{

    connection_manager::connection_manager()
    {
    }

    void connection_manager::start(connection_ptr c)
    {
        {
            std::lock_guard<std::mutex> guard(con_mutex_);
            connections_.insert(c);
        }
        c->start();
    }

    void connection_manager::stop(connection_ptr c)
    {
        {
            std::lock_guard<std::mutex> guard(con_mutex_);
            connections_.erase(c);
        }

        c->stop();
    }

    void connection_manager::stop_all()
    {
        std::vector<connection_ptr> con_copys;
        {
            std::lock_guard<std::mutex> guard(con_mutex_);
            con_copys.insert(con_copys.end(), connections_.begin(), connections_.end());
            connections_.clear();
        }
        for (auto c : con_copys)
        {
            c->stop();
        }
    }
    std::size_t connection_manager::get_connection_count()
    {
        std::lock_guard<std::mutex> guard(con_mutex_);
        return connections_.size();
    }

} // namespace spiritsaway::http_server
