#include <iostream>
#include "srpc/common/layer.h"
#include "srpc/common/layer_list.h"

#include "google/protobuf/service.h"

namespace srpc { namespace server {

}}

namespace srpc { namespace client {

}}

namespace srpc { namespace common {

	class errro_type {
	public:
	private:
	};

	class service {

	};

	template <typename MessageType>
	class service_executor {

	public:
		using message_type = MessageType;
		void make_call(message_type msg)
		{
			
		}
	private:
	};

	template <typename MessageType, typename ServiceExecutorType>
	class executor_layer: public srpc::layer<MessageType> {
	public:
		using service_executor = ServiceExecutorType;
		using message_type = MessageType;

		executor_layer() = default;
		executor_layer(srpc::layer<MessageType> *lower_layers)
		{
			set_lower(lower_layers);
		}
		
		service_executor &get_executor() 
		{
			return executor_;
		}

		const service_executor &get_executor() const
		{
			return executor_;
		}
	private:
		void from_upper(message_type msg)
		{
			this->send_to_lower(std::move(msg));
		}

		void from_lower(message_type msg)
		{
			executor_.make_call(std::move(msg));
		}
		service_executor executor_;
	};

	template <typename MessageType, typename ServiceExecutor>
	class connection_info {

	public:

		using message_type = MessageType;
		using service_executor = ServiceExecutor;
		using this_type = connection_info<message_type, service_executor>;
		using protocol_layer_type = srpc::layer_list<message_type>;
		using executor_layer_type = executor_layer<message_type, service_executor>;

		virtual std::string name() const = 0;
		virtual std::uintptr_t handle() const = 0;

		virtual ~connection_info() = default;

		connection_info()
		{
			executor_.set_lower(&protocol_);
		}

		protocol_layer_type &get_protocol_layer()
		{
			return protocol_;
		}

		executor_layer_type &get_executor()
		{
			return executor_;
		}

	private:
		executor_layer_type executor_;
		protocol_layer_type protocol_;
	};
}}

using namespace srpc::common;

int main() 
{
	using message_type = std::string;
	
	class executor: public srpc::pass_through_layer<message_type> {
	public:
		void from_lower(message_type msg) override
		{
			std::cout << "exe " << msg << "\n";
		}
	};

	class print: public srpc::layer<message_type> {
		void from_upper(message_type msg) override
		{
			std::cout << msg << "\n";
			if(has_lower()) {
				send_to_lower("-> " + msg);
			}
		}
		void from_lower(message_type msg) override
		{
			std::cout << "<" << msg << "\n";
			send_to_upper("<-" + msg);
		}
	};

	class protocol: public srpc::layer_list<message_type> { };

	executor exe;
	protocol proto;

	exe.set_lower(&proto);
	proto.set_upper(&exe);
	proto.create_back<print>();

	exe.from_upper("!");
	proto.from_lower("!");

	return 0;

	//struct executor {
	//	void make_call(int) {}
	//};
	//class connection: public connection_info<int, executor> {
	//	std::string name() const override 
	//	{
	//		return "int runner";
	//	}
	//	std::uintptr_t handle() const override 
	//	{
	//		return 0;
	//	}
	//};
	return 0;
}