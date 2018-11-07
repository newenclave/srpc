#include "srpc/common/executor_layer.h"
#include "srpc/common/layer.h"
#include "srpc/common/layer_list.h"

#include <iostream>
#include <thread>
#include "srpc/common/protobuf/protocol/lowlevel.pb.h"

//#include "google/protobuf/service.h"

namespace srpc { namespace server {

}}

namespace srpc { namespace client {

}}

namespace srpc { namespace common {

    template <typename ReqType, typename ResType, typename ServiceExecutor>
    class connection_info {

    public:
        using req_type = ReqType;
        using res_type = ResType;
        using service_executor = ServiceExecutor;
        using this_type = connection_info<req_type, res_type, service_executor>;
        using protocol_layer_type = layer_list<req_type, res_type>;
        using executor_layer_type
            = executor_layer<req_type, res_type, service_executor>;

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

struct request {
    request(std::string cmd, std::string dat)
        : command(std::move(cmd))
        , data(std::move(dat))
    {
    }
    std::string command;
    std::string data;
};


struct response {
    response(int c)
        : code(c)
    {}
    int code = 0;
};

template <typename ConnTrait>
class executor {
public:
    using connection_trait = ConnTrait;
    using connection_type = typename connection_trait::connection_type;
    executor() = default;
    void make_call(request msg)
    {
        std::cout << "c: " << msg.command << "\n";
        std::cout << "d: " << msg.data << "\n";
        if (msg.command == "-") {
            cnt_->get_executor_layer().from_upper(response(-1));
        } else if (msg.command == "q") {
            // cnt_->stop();
        } else {
            cnt_->get_executor_layer().from_upper(response(0));
        }
    }

    void set_connection_info(connection_type* cnt)
    {
        cnt_ = cnt;
    }

private:
    connection_type* cnt_;
};

class parse_layer : public srpc::common::layer<request, response> {
    void from_upper(response msg) override
    {
    }
    void from_lower(request msg) override
    {
        msg.command.assign(msg.data.begin(), msg.data.begin() + 1);
        msg.data.assign(msg.data.begin() + 1, msg.data.end());
        send_to_upper(std::move(msg));
    }
};

class print_console_layer : public srpc::common::layer<request, response> {
    void from_upper(response msg) override
    {
        if (msg.code) {
            std::cout << "error: " << msg.code << "\n";
        } else {
            std::cout << "Ok\n";
        }
    }
    void from_lower(request msg) override
    {
        send_to_upper(std::move(msg));
    }
};

class connection;

struct trait {
    using connection_type = connection;
};

using executor_type = executor<trait>;

class connection : public connection_info<request, response, executor_type> {

public:
    connection() {}

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
            con.get_protocol_layer().from_lower(request("", command ));
        }
    });

    t.join();

    return 0;
}
