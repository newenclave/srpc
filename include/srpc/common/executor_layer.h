#pragma once

#include "layer.h"

namespace srpc { namespace common {

    template <typename ReqType, typename ResType,
              typename ServiceExecutorType>
    class executor_layer : public layer<ReqType, ResType> {
    public:
        using service_executor = ServiceExecutorType;
        using req_type = ReqType;
        using res_type = ResType;

        executor_layer() = default;

        service_executor& get_executor()
        {
            return executor_;
        }

        const service_executor& get_executor_layer() const
        {
            return executor_;
        }

    public:
        void from_upper(res_type msg)
        {
            this->send_to_lower(std::move(msg));
        }

        void from_lower(req_type msg)
        {
            executor_.make_call(std::move(msg));
        }
        service_executor executor_;
    };
}}
