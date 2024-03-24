#include "json.hpp"
#include "fifo_map.hpp"

template<typename T>
using fifo_dict = nlohmann::fifo_map<std::string, T>;
template<class K, class V, class dummy_compare, class A>
using dummy_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using fifo_json = nlohmann::basic_json<dummy_fifo_map>;