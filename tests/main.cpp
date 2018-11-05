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

	template <typename MessageType>
	class connection_info: public std::enable_shared_from_this<connection_info<MessageType> > {
	public:
		using message_type = MessageType;
		template <typename T, typename ...Args>
		static 
		std::shared_ptr<connection_info<message_type> > create(Args&...args) {
			static_assert(std::is_base_of<connection_info, T>::value, 
				"T is not delivered from 'srpc::common::connection_info'");
			auto info = std::make_shared<T>(std::forward<Args>(args)...);
			return info;
		}
	protected:
		virtual ~connection_info() = default;
	private:
		srpc::layer_list<message_type> protocol_;
	};

}}

int main() 
{
	class ci: public srpc::common::connection_info<int> {};
	auto cisp = srpc::common::connection_info<int>::create<ci>();
	return 0;
}