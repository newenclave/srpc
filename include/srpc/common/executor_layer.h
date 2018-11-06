#pragma once

#include "layer.h"

namespace srpc { namespace common {

    template <typename MessageType, typename ServiceExecutorType>
    class executor_layer : public layer<MessageType> {
    public:
        using service_executor = ServiceExecutorType;
        using message_type = MessageType;

        executor_layer() = default;

        service_executor& get_executor_layer()
        {
            return executor_;
        }

        const service_executor& get_executor_layer() const
        {
            return executor_;
        }

    public:
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
}}