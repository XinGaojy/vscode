#pragma once
#include <yaml-cpp/yaml.h>
#include <vector>

namespace YAML {
// 特化YAML转换（示例：std::vector<int>）
template<>
struct convert<std::vector<int>> {
    static bool decode(const Node& node, std::vector<int>& rhs) {
        if (!node.IsSequence()) return false;
        rhs.clear();
        for (const auto& item : node) {
            rhs.push_back(item.as<int>());
        }
        return true;
    }
};
} // namespace YAML