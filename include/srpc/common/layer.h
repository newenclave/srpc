#pragma once

#include <memory>

namespace srpc { namespace common {

    namespace traits {
        struct raw_pointer {
            template <typename T>
            using pointer_type = T*;

            template <typename T>
            static bool is_empty(pointer_type<T> ptr)
            {
                return ptr == nullptr;
            }

            template <typename T>
            static pointer_type<T> get_write(pointer_type<T> ptr)
            {
                return ptr;
            }

            template <typename T>
            static void destroy(pointer_type<T>)
            {
            }

            template <typename T>
            static void swap(pointer_type<T>& lh, pointer_type<T>& rh)
            {
                std::swap(lh, rh);
            }
        };

        struct unique_pointer {
            template <typename T>
            using pointer_type = std::unique_ptr<T>;

            template <typename T>
            static bool is_empty(pointer_type<T>& ptr)
            {
                return ptr == nullptr;
            }

            template <typename T>
            static pointer_type<T>& get_write(pointer_type<T>& ptr)
            {
                return ptr;
            }

            template <typename T>
            static void destroy(pointer_type<T>& ptr)
            {
                ptr.reset();
            }

            template <typename T>
            static void swap(pointer_type<T>& lh, pointer_type<T>& rh)
            {
                std::swap(lh, rh);
            }
        };
    }

    template <typename ReqType, typename ResType,
              typename UpperPtrTrait = traits::raw_pointer,
              typename LowerPtrTrait = UpperPtrTrait>
    class layer {
        using upper_traits = UpperPtrTrait;
        using lower_traits = LowerPtrTrait;
        using this_type = layer<ReqType, ResType, upper_traits, lower_traits>;

    public:
        using req_type = ReqType;
        using res_type = ResType;
        using upper_pointer_type =
            typename upper_traits::template pointer_type<this_type>;
        using lower_pointer_type =
            typename lower_traits::template pointer_type<this_type>;

        virtual ~layer()
        {
            upper_traits::destroy(upper_);
            lower_traits::destroy(lower_);
        }

        layer() = default;

        layer(this_type&& other)
        {
            swap(other);
        }

        layer& operator=(this_type&& other)
        {
            swap(other);
            return *this;
        }

        void swap(this_type& other)
        {
            upper_traits::swap(upper_, other.upper_);
            lower_traits::swap(lower_, other.lower_);
        }

        virtual void set_upper(upper_pointer_type upper)
        {
            std::swap(upper_, upper);
        }

        virtual void set_lower(lower_pointer_type lower)
        {
            std::swap(lower_, lower);
        }

    public:
        virtual void from_upper(res_type msg) = 0; // from upper layer

        virtual void from_lower(req_type msg) = 0; // from lower layer

    protected:
        void send_to_lower(res_type msg)
        {
            lower_traits::get_write(lower_)->from_upper(std::move(msg));
        }

        void send_to_upper(req_type msg)
        {
            upper_traits::get_write(upper_)->from_lower(std::move(msg));
        }

        bool has_upper() const
        {
            return !upper_traits::is_empty(upper_);
        }

        bool has_lower() const
        {
            return !lower_traits::is_empty(lower_);
        }

        upper_pointer_type& get_upper()
        {
            return upper_;
        }

        lower_pointer_type& get_lower()
        {
            return lower_;
        }

    private:
        upper_pointer_type upper_ = nullptr;
        lower_pointer_type lower_ = nullptr;
    };

    template <typename ReqType, typename ResType,
              typename UpperPtrTrait = traits::raw_pointer,
              typename LowerPtrTrait = UpperPtrTrait>
    class pass_through_layer
        : public layer<ReqType, ResType, UpperPtrTrait, LowerPtrTrait> {

        using upper_traits = UpperPtrTrait;
        using lower_traits = LowerPtrTrait;
        using this_type
            = pass_through_layer<ReqType, ResType, upper_traits, lower_traits>;
        using super_type = layer<ReqType, ResType, upper_traits, lower_traits>;

    public:
        using req_type = typename super_type::req_type;
        using res_type = typename super_type::res_type;
        using upper_pointer_type = typename super_type::upper_pointer_type;
        using lower_pointer_type = typename super_type::lower_pointer_type;

    public:
        void from_upper(res_type msg) override // from upper layer
        {
            this->send_to_lower(std::move(msg));
        }

        void from_lower(req_type msg) override // from lower layer
        {
            this->send_to_upper(std::move(msg));
        }
    };

}}
