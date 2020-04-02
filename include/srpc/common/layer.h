#pragma once
#include "srpc/common/slot.h"

namespace srpc { namespace common {
    /*
            upper_slot
                v
        +-------|-------+
        |  read_upper() |
        |     layer     |
        |  read_lower() |
        +-------|-------+
                ^
            lower_slot
    */


    template <typename UpperT, typename LowerT>
    struct layer {
    private:
        struct upper_slot_impl : public slot<UpperT> {
            void write(UpperT msg) override
            {
                parent_->read_upper(std::move(msg));
            }
            layer *parent_ = nullptr;
        };

        struct lower_slot_impl : public slot<LowerT> {
            void write(LowerT msg) override
            {
                parent_->read_lower(std::move(msg));
            }
            layer *parent_ = nullptr;
        };

    public:
        virtual ~layer() = default;

        layer()
        {
            upper_slot_.parent_ = this;
            lower_slot_.parent_ = this;
        }

        layer(layer &&other)
        {
            upper_slot_.parent_ = this;
            lower_slot_.parent_ = this;
            swap(other);
        }

        layer &operator=(layer &&other)
        {
            swap(other);
        }

        layer(const layer &other) = delete;
        layer &operator=(const layer &other) = delete;

        virtual void read_upper(UpperT mgs) = 0;
        virtual void read_lower(LowerT mgs) = 0;

        slot<UpperT> &upper_slot()
        {
            return upper_slot_;
        }

        slot<LowerT> &lower_slot()
        {
            return lower_slot_;
        }

        template <typename LT>
        layer<LowerT, LT> &connect(layer<LowerT, LT> &other)
        {
            lower_ = &other.upper_slot();
            other.set_upper(&lower_slot_);
            return other;
        }

        void set_upper(slot<UpperT> *value)
        {
            upper_ = value;
        }
        void set_lower(slot<LowerT> *value)
        {
            lower_ = value;
        }

        void swap(layer &other)
        {
            std::swap(upper_, other.upper_);
            std::swap(lower_, other.lower_);
        }

    protected:
        slot<UpperT> *upper_ = nullptr;
        slot<LowerT> *lower_ = nullptr;

    private:
        upper_slot_impl upper_slot_;
        lower_slot_impl lower_slot_;
    };
}}