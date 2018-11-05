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

		executor_layer()
		{}
		
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
	class connection_info: 
		public std::enable_shared_from_this<
			connection_info<
				MessageType, 
				ServiceExecutor
			> > {
	public:
		using message_type = MessageType;
		using service_executor = ServiceExecutor;
		using this_type = connection_info<message_type, service_executor>;
		template <typename T, typename ...Args>
		static 
		std::shared_ptr<this_type> create(Args&...args) {
			static_assert(std::is_base_of<connection_info, T>::value, 
				"T is not delivered from 'srpc::common::connection_info'");
			auto info = std::make_shared<T>(std::forward<Args>(args)...);
			using executor_layer_type = executor_layer<message_type, service_executor>;
			info->protocol_.create_back<executor_layer_type>();
			return info;
		}

		virtual std::string name() const = 0;
		virtual std::uintptr_t handle() const = 0;

	protected:
		virtual ~connection_info() = default;
	private:
		srpc::layer_list<message_type> protocol_;
	};

}}

using namespace srpc::common;

int main() 
{
	struct executor {
		void make_call(int) {}
	};
	class connection: public connection_info<int, executor> {
		std::string name() const override 
		{
			return "int runner";
		}
		std::uintptr_t handle() const override 
		{
			return 0;
		}
	};
	auto ci = connection::create<connection>();
	return 0;
}