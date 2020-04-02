#pragma once
namespace srpc { namespace common {

    template <typename T>
    struct slot {
        virtual ~slot() = default;
        virtual void write(T) = 0;
    };

}}