#pragma once
#include "srpc/common/slot.h"

namespace srpc { namespace common {

    /*
             upper_slot
                  v
        +---------|--------+
        |  on_upper_data() |
        |       layer      |
        |  on_lower_data() |
        +---------|--------+
                  ^
             lower_slot
    */
    template <typename UpperT, typename LowerT>
    class layer;

    // template <typename... Args>
    // class layer_list;

    template <typename UpperT, typename LowerT>
    class layer {
    private:
        struct upper_slot_impl : public slot<UpperT> {
            upper_slot_impl() = default;
            upper_slot_impl(upper_slot_impl &&) {}
            upper_slot_impl &operator=(upper_slot_impl &&)
            {
                return *this;
            }
            void write(UpperT msg) override
            {
                parent_->on_upper_data(std::move(msg));
            }
            layer *parent_ = nullptr;
        };

        struct lower_slot_impl : public slot<LowerT> {
            lower_slot_impl() = default;
            lower_slot_impl(lower_slot_impl &&) {}
            lower_slot_impl &operator=(lower_slot_impl &&)
            {
                return *this;
            }
            void write(LowerT msg) override
            {
                parent_->on_lower_data(std::move(msg));
            }
            layer *parent_ = nullptr;
        };

    public:
        using upper_type = UpperT;
        using lower_type = LowerT;

        virtual ~layer() = default;

        layer()
        {
            set_slots();
        }

        layer(layer &&other)
        {
            set_slots();
        }

        layer &operator=(layer &&other)
        {
            return *this;
        }

        layer(const layer &other)
        {
            set_slots();
        }

        layer &operator=(const layer &other)
        {
            return *this;
        }

        virtual void on_upper_data(UpperT mgs) = 0;
        virtual void on_lower_data(LowerT mgs) = 0;

        slot<UpperT> &upper_slot()
        {
            return upper_slot_;
        }

        slot<LowerT> &lower_slot()
        {
            return lower_slot_;
        }

        void connect_lower(slot<LowerT> &target_lower_slot)
        {
            lower_ = &target_lower_slot;
        }

        void connect_upper(slot<UpperT> &target_upper_slot)
        {
            upper_ = &target_upper_slot;
        }

    protected:
        bool has_upper() const
        {
            return upper_ != nullptr;
        }

        bool has_lower() const
        {
            return lower_ != nullptr;
        }

        void write_up(UpperT msg)
        {
            upper_->write(std::move(msg));
        }

        void write_down(LowerT msg)
        {
            lower_->write(std::move(msg));
        }

    private:
        slot<UpperT> *upper_ = nullptr;
        slot<LowerT> *lower_ = nullptr;

        void set_slots()
        {
            upper_slot_.parent_ = this;
            lower_slot_.parent_ = this;
        }

        upper_slot_impl upper_slot_;
        lower_slot_impl lower_slot_;
    };

#if 0
    template <typename LowerT>
    struct layer<void, LowerT> {
    private:
        struct lower_slot_impl : public slot<LowerT> {
            void write(LowerT msg) override
            {
                parent_->read_lower(std::move(msg));
            }
            layer *parent_ = nullptr;
        };

    public:
        using upper_type = void;
        using lower_type = LowerT;

        virtual ~layer() = default;

        layer()
        {
            lower_slot_.parent_ = this;
        }

        layer(layer &&other)
        {
            lower_slot_.parent_ = this;
        }

        layer &operator=(layer &&other)
        {
            return *this;
        }

        layer(const layer &other)
        {
            lower_slot_.parent_ = this;
        }

        layer &operator=(const layer &other)
        {
            return *this;
        }

        virtual void read_lower(LowerT mgs) = 0;

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

        bool has_lower() const
        {
            return lower_ != nullptr;
        }

        void write_down(LowerT msg)
        {
            lower_->write(std::move(msg));
        }

        void set_lower(slot<LowerT> *value)
        {
            lower_ = value;
        }

    protected:
        slot<LowerT> *lower_ = nullptr;

    private:
        lower_slot_impl lower_slot_;
    };

    template <typename UpperT>
    struct layer<UpperT, void> {
    private:
        struct upper_slot_impl : public slot<UpperT> {
            void write(UpperT msg) override
            {
                parent_->read_upper(std::move(msg));
            }
            layer *parent_ = nullptr;
        };

    public:
        using upper_type = UpperT;
        using lower_type = void;

        virtual ~layer() = default;

        layer()
        {
            upper_slot_.parent_ = this;
        }

        layer(layer &&other)
        {
            upper_slot_.parent_ = this;
        }

        layer &operator=(layer &&other)
        {
            return *this;
        }

        layer(const layer &other)
        {
            upper_slot_.parent_ = this;
        }

        layer &operator=(const layer &other)
        {
            return *this;
        }

        virtual void read_upper(UpperT mgs) = 0;

        slot<UpperT> &upper_slot()
        {
            return upper_slot_;
        }

        bool has_upper() const
        {
            return upper_ != nullptr;
        }

        void write_up(UpperT msg)
        {
            upper_->write(std::move(msg));
        }

        void set_upper(slot<UpperT> *value)
        {
            upper_ = value;
        }

    protected:
        slot<UpperT> *upper_ = nullptr;

    private:
        upper_slot_impl upper_slot_;
    };
#endif

}}
