#pragma once
namespace srpc { namespace common {

    template <typename T>
    struct slot {
        virtual ~slot() = default;
        virtual void write(T) = 0;
    };

    template <typename T, typename ParentT, void (ParentT::*Call)(T)>
    class delegate_slot : public slot<T> {
    public:
        delegate_slot(ParentT *parent)
            : parent_(parent)
        {
        }
        delegate_slot(delegate_slot &&) {}
        delegate_slot(const delegate_slot &) {}
        delegate_slot &operator=(delegate_slot &&)
        {
            return *this;
        }
        delegate_slot &operator=(const delegate_slot &)
        {
            return *this;
        }
        void write(T msg) override
        {
            (parent_->*Call)(std::move(msg));
        }

    private:
        ParentT *parent_ = nullptr;
    };
}}
