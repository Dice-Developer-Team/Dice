#pragma once
#include "yaml-cpp/yaml.h"
namespace YAML
{
    template <typename T>
    struct as_if<T, std::optional<T>>
    {
        explicit as_if(const Node& node_) : node(node_) {}
        const Node& node;

        const std::optional<T> operator()() const
        {
            std::optional<T> val;
            T t;
            if (node.m_pNode && convert<T>::decode(node, t))
                val = std::move(t);

            return val;
        }
    };

    // There is already a std::string partial specialisation, so we need a full specialisation here
    template <>
    struct as_if<std::string, std::optional<std::string>>
    {
        explicit as_if(const Node& node_) : node(node_) {}
        const Node& node;

        const std::optional<std::string> operator()() const
        {
            std::optional<std::string> val;
            std::string t;
            if (node.m_pNode && convert<std::string>::decode(node, t))
                val = std::move(t);

            return val;
        }
    };
}
//std::optional<bool> as_bool = YAML::as_if<bool, std::optional<bool> >(node)();
//std::optional<int> as_int = YAML::as_if<int, std::optional<int> >(node)();
//std::optional<double> as_double = YAML::as_if<double, std::optional<double> >(node)();
//std::optional<std::string> as_string = YAML::as_if<std::string, std::optional<std::string> >(node)();