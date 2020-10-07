#pragma once
namespace srpc { namespace common {

    template <typename T>
    struct slot {
        virtual ~slot() = default;
        virtual void write(T) = 0;
    };

    template <typename T>
    class function_slot : public slot<T> {
    public:
        template <typename... Args>
        explicit function_slot(Args &&...params)
            : call_(std::forward<Args>(params)...)
        {
        }
        function_slot(function_slot &&) {}
        function_slot(const function_slot &) {}
        function_slot &operator=(function_slot &&)
        {
            return *this;
        }
        function_slot &operator=(const function_slot &)
        {
            return *this;
        }
        void write(T msg) override
        {
            call_(std::move(msg));
        }
    private:
        std::function<void(T)> call_;
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
