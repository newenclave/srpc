#include "srpc/common/executor_layer.h"
#include "srpc/common/layer.h"
#include "srpc/common/layer_list.h"

#include <iostream>
#include <thread>
#include "srpc/common/protobuf/protocol/lowlevel.pb.h"

#include "google/protobuf/service.h"

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

struct message {
	srpc::rpc::lowlevel ll;
};

template <typename ConnTrait>
class executor {
public:
    using connection_trait = ConnTrait;
    using connection_type = typename connection_trait::connection_type;
    executor() = default;
    void make_call(message msg)
    {
		auto srv_itr = services_.find(msg.ll.call().service_id());
		int error = 200;
		if(srv_itr != services_.end()) {
			auto method = srv_itr->second->GetDescriptor()->FindMethodByName(msg.ll.call().method_id());
			if(method) {
				auto request = srv_itr->second->GetRequestPrototype().New(&arena_);
				auto response = srv_itr->second->GetResponsePrototype().New(&arena_);
				request->ParseFromString(msg.ll.request());
				srv_itr->second->CallMethod(method, nullptr, request, response, nullptr);
				msg.ll.clear_request();
				msg.ll.set_response(response->SerializeAsString());
			} else {
				error = 401;
			}
		} else {
			error = 404;
		}
		if(error !=  200) {
			srpc::rpc::lowlevel error;
			error.set_id(msg.ll.id());
			error.mutable_error()->set_code(error);
		}
    }

    void set_connection_info(connection_type* cnt)
    {
        cnt_ = cnt;
    }

	template <typename Svc, typename ...Args>
	void add_service(Args &&...args) 
	{
		auto svc = std::make_unique<Svc>(std::forward<Args>(args)...);
		std::string name = svc->GetDescriptor().full_name();
		services_.emplace(std::make_pair(std::move(name), std::move(svc)));
	}

private:
    connection_type* cnt_;
    std::map<std::string, std::unique_ptr<google::protobuf::Service> > services_;
    google::protobuf::Arena arena_;
};

class parse_layer : public srpc::common::layer<message, message> {
    void from_upper(message msg) override
    {
    }
    void from_lower(message msg) override
    {
    }
};

class print_console_layer : public srpc::common::layer<message, message> {
    void from_upper(message msg) override
    {
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

class connection : public connection_info<message, message, executor_type> {

public:
    connection() {}

    void init()
    {
        get_executor_layer().get_executor().set_connection_info(this);
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
			con.get_protocol_layer().from_lower({});
        }
    });

    t.join();

    return 0;
}
