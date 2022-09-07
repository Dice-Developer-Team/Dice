#include "json.hpp"
#include "fifo_map.hpp"

template<class K, class V, class dummy_compare, class A>
using dummy_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using fifo_json = nlohmann::basic_json<dummy_fifo_map>;