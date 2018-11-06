#pragma once
#include "layer.h"
#include <list>

namespace srpc { namespace common {

    template <typename MsgType>
    class layer_list
        : public layer<MsgType, traits::raw_pointer, traits::raw_pointer> {
    public:
        using message_type = MsgType;
        using super_type
            = layer<MsgType, traits::raw_pointer, traits::raw_pointer>;
        using layer_type = layer<MsgType>;
        using upper_pointer_type = typename layer_type::upper_pointer_type;
        using lower_pointer_type = typename layer_type::lower_pointer_type;
        using layer_uptr = std::unique_ptr<layer_type>;
        using iterator = typename std::list<layer_uptr>::iterator;
        using const_iterator = typename std::list<layer_uptr>::const_iterator;

        iterator begin()
        {
            return list_.begin();
        }

        iterator end()
        {
            return list_.end();
        }

        const_iterator cbegin() const
        {
            return list_.cbegin();
        }

        const_iterator cend() const
        {
            return list_.cend();
        }

    public:
        void from_upper(message_type msg) override
        {
            list_.front()->from_upper(std::move(msg));
        }

        void from_lower(message_type msg) override
        {
            list_.back()->from_lower(std::move(msg));
        }

        void push_front(layer_uptr new_value)
        {
            layer_type* old
                = list_.empty() ? this->get_lower() : list_.front().get();
            new_value->set_lower(old);
            if (old) {
                old->set_upper(new_value.get());
            }
            list_.push_front(std::move(new_value));
        }

        template <typename LT, typename... Args>
        void create_front(Args&&... args)
        {
            auto nv = layer_uptr(new LT(std::forward<Args>(args)...));
            push_front(std::move(nv));
        }

        void push_back(layer_uptr new_value)
        {
            layer_type* old
                = list_.empty() ? this->get_upper() : list_.back().get();
            new_value->set_upper(old);
            if (old) {
                old->set_lower(new_value.get());
            }
            list_.push_back(std::move(new_value));
        }

        template <typename LT, typename... Args>
        void create_back(Args&&... args)
        {
            auto nv = layer_uptr(new LT(std::forward<Args>(args)...));
            push_back(std::move(nv));
        }

        std::size_t size() const
        {
            return list_.size();
        }

        std::size_t empty() const
        {
            return list_.empty();
        }

        void set_upper(upper_pointer_type ptr) override
        {
            auto upper_ptr = list_.empty() ? nullptr : list_.front().get();
            if (upper_ptr) {
                upper_ptr->set_upper(ptr);
            }
            super_type::set_upper(ptr);
        }

        void set_lower(lower_pointer_type ptr) override
        {
            auto lower_ptr = list_.empty() ? nullptr : list_.back().get();
            if (lower_ptr) {
                lower_ptr->set_lower(ptr);
            }
            super_type::set_lower(ptr);
        }

    private:
        std::list<layer_uptr> list_;
    };

}}
