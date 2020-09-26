#pragma once

#include "srpc/common/layer.h"
#include <functional>
#include <tuple>

namespace srpc { namespace common {

    namespace utils {
#define LAYER_UTILS_ENABLE_IF(cond) std::enable_if_t<(cond), bool> = true

        template <int Id, typename T, LAYER_UTILS_ENABLE_IF(Id == 0)>
        void connect_tuple_impl(T &value)
        {
        }

        template <int Id, typename T, LAYER_UTILS_ENABLE_IF(Id > 0)>
        void connect_tuple_impl(T &value)
        {
            std::get<Id - 1>(value).connect_lower( //-V104
                std::get<Id>(value).upper_slot());

            std::get<Id>(value).connect_upper(
                std::get<Id - 1>(value).lower_slot()); //-V104

            connect_tuple_impl<Id - 1>(value);
        }

        template <typename... Args>
        void connect_tuple(std::tuple<Args...> &value)
        {
            connect_tuple_impl<sizeof...(Args) - 1>(value);
        }

#undef LAYER_UTILS_ENABLE_IF
    }


    //: public layer<
    //      typename std::tuple_element_t<0, std::tuple<Args...>>::upper_type,
    //      typename std::tuple_element_t<sizeof...(Args) - 1,
    //                                    std::tuple<Args...>>::lower_type>

    template <typename... Args>
    class layer_list {

    public:
        static const std::size_t last_index = sizeof...(Args) - 1;

        using first_element = std::tuple_element_t<0, std::tuple<Args...>>;
        using last_element
            = std::tuple_element_t<last_index, std::tuple<Args...>>;

        using upper_layer = layer<typename first_element::upper_type,
                                  typename first_element::lower_type>;

        using lower_layer = layer<typename last_element::upper_type,
                                  typename last_element::lower_type>;

        using upper_type = typename upper_layer::upper_type;
        using lower_type = typename lower_layer::lower_type;

    private:
        struct upper_slot_impl : public slot<upper_type> {
            upper_slot_impl() = default;
            upper_slot_impl(upper_slot_impl &&) {};
            upper_slot_impl &operator=(upper_slot_impl &&)
            {
                return *this;
            }
            void write(upper_type message)
            {
                parent_->translate_upper(std::move(message));
            }
            layer_list *parent_ = nullptr;
        };

        struct lower_slot_impl : public slot<lower_type> {
            lower_slot_impl() = default;
            lower_slot_impl(lower_slot_impl &&) {};
            lower_slot_impl &operator=(lower_slot_impl &&)
            {
                return *this;
            }
            void write(lower_type message)
            {
                parent_->translate_lower(std::move(message));
            }
            layer_list *parent_ = nullptr;
        };

        void translate_upper(upper_type msg)
        {
            on_upper_ready_(std::move(msg));
        }

        void translate_lower(lower_type msg)
        {
            on_lower_ready_(std::move(msg));
        }

    public:
        layer_list(const layer_list &) = delete;
        layer_list &operator=(const layer_list &) = delete;
        layer_list() = delete;

        layer_list(std::tuple<Args...> layers)
            : layers_(std::move(layers))
        {
            connect_self();
        }

        template <typename... LayersL>
        layer_list(LayersL &&... layers)
            : layer_list(std::make_tuple(std::forward<LayersL>(layers)...))
        {
        }

        layer_list(layer_list &&other)
            : layers_(std::move(other.layers_))
        {
            connect_self();
            on_lower_ready_ = std::move(other.on_lower_ready_);
            on_upper_ready_ = std::move(other.on_upper_ready_);
        }

        layer_list &operator=(layer_list &&other)
        {
            layers_ = std::move(other.layers_);
            connect_self();
            on_lower_ready_ = std::move(other.on_lower_ready_);
            on_upper_ready_ = std::move(other.on_upper_ready_);
            return *this;
        }

        void write_upper(upper_type msg)
        {
            get<0>().upper_slot().write(std::move(msg));
        }

        void write_lower(upper_type msg)
        {
            get<last_index>().lower_slot().write(std::move(msg));
        }

        template <int Id>
        auto &get()
        {
            return std::get<Id>(layers_);
        }

        void on_upper_ready_connect(std::function<void(upper_type)> val)
        {
            on_upper_ready_ = std::move(val);
        }

        void on_lower_ready_connect(std::function<void(lower_type)> val)
        {
            on_lower_ready_ = std::move(val);
        }

    private:
        void connect_self()
        {
            utils::connect_tuple(layers_);
            upper_connector_.parent_ = this;
            lower_connector_.parent_ = this;
            get<0>().connect_upper(upper_connector_);
            get<last_index>().connect_lower(lower_connector_);
        }

        upper_slot_impl upper_connector_;
        lower_slot_impl lower_connector_;
        std::tuple<Args...> layers_;
        std::function<void(upper_type)> on_upper_ready_ = [](upper_type) {};
        std::function<void(lower_type)> on_lower_ready_ = [](lower_type) {};
    };

    template <typename... Args>
    inline auto make_layer_list(Args &&... args)
    {
        return layer_list<Args...>(std::forward<Args>(args)...);
    }
}}
