#include "srpc/common/executor_layer.h"
#include "srpc/common/layer.h"
#include "srpc/common/layer_list.h"

#include <iostream>
#include <thread>

#include "google/protobuf/service.h"

namespace srpc { namespace server {

}}

namespace srpc { namespace client {

}}

namespace srpc { namespace common {

    template <typename MessageType, typename ServiceExecutor>
    class connection_info {

    public:
        using message_type = MessageType;
        using service_executor = ServiceExecutor;
        using this_type = connection_info<message_type, service_executor>;
        using protocol_layer_type = srpc::common::layer_list<message_type>;
        using executor_layer_type
            = executor_layer<message_type, service_executor>;

        virtual std::string name() const = 0;
        virtual std::uintptr_t handle() const = 0;

        virtual ~connection_info() = default;

        connection_info()
        {
            executor_.set_lower(&protocol_);
            protocol_.set_upper(&executor_);
        }

        protocol_layer_type& get_protocol_layer()
        {
            return protocol_;
        }

        executor_layer_type& get_executor_layer()
        {
            return executor_;
        }

    private:
        executor_layer_type executor_;
        protocol_layer_type protocol_;
    };
}}

using namespace srpc::common;

struct message {
    message(int c, std::string cmd, std::string dat)
        : code(c)
        , command(std::move(cmd))
        , data(std::move(dat))
    {
    }
    int code = 0;
    std::string command;
    std::string data;
};

template <typename ConnTrait>
class executor {
public:
    using connection_trait = ConnTrait;
    using connection_type = typename connection_trait::connection_type;
    executor() = default;
    void make_call(message msg)
    {
        std::cout << "c: " << msg.command << "\n";
        std::cout << "d: " << msg.data << "\n";
        if (msg.command == "-") {
            cnt_->get_executor_layer().from_upper(message( -1, "-", "failed" ));
        } else if (msg.command == "q") {
            // cnt_->stop();
        } else {
            cnt_->get_executor_layer().from_upper(message( 0, "+", "" ));
        }
    }

    void set_connection_info(connection_type* cnt)
    {
        cnt_ = cnt;
    }

private:
    connection_type* cnt_;
};

class parse_layer : public srpc::common::layer<message> {
    void from_upper(message msg) override
    {
        msg.command.append(" ");
        msg.command.append(msg.data);
        msg.data.swap(msg.command);
    }
    void from_lower(message msg) override
    {
        msg.command.assign(msg.data.begin(), msg.data.begin() + 1);
        msg.data.assign(msg.data.begin() + 1, msg.data.end());
        send_to_upper(std::move(msg));
    }
};

class print_console_layer : public srpc::common::layer<message> {
    void from_upper(message msg) override
    {
        if (msg.code) {
            std::cout << "error: " << msg.data << "\n";
        } else {
            std::cout << "Ok: " << msg.data << "\n";
        }
    }
    void from_lower(message msg) override
    {
        send_to_upper(std::move(msg));
    }
};

class connection;

struct trait {
    using connection_type = connection;
};

using executor_type = executor<trait>;

class connection : public connection_info<message, executor_type> {

public:
    connection() {}

    std::string name() const
    {
        return "simple console client";
    }
    std::uintptr_t handle() const
    {
        return 0;
    }

    void init()
    {
        get_executor_layer().get_executor_layer().set_connection_info(this);
        active_ = true;
    }

    void stop()
    {
        active_ = false;
    }

    bool active() const
    {
        return active_;
    }

private:
    bool active_ = false;
};

int main()
{

    connection con;
    con.get_protocol_layer().create_back<print_console_layer>();
    con.get_protocol_layer().create_back<parse_layer>();
    con.init();

    std::thread t([&]() {
        while (true) {
            std::string command;
            std::getline(std::cin, command);
            con.get_protocol_layer().from_lower(message( 0, "", command ));
        }
    });

    t.join();

    return 0;
}
