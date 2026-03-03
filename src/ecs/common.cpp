module;

#include <cstddef>

#include <boost/container/vector.hpp>
#include <boost/container_hash/hash.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

export module Ecs:Common;

export namespace bstc = boost::container;
export namespace bstu = boost::unordered;

export namespace boost {
namespace container {
using ::boost::container::vector;
}
namespace unordered {
using ::boost::unordered::unordered_flat_map;
}
using ::boost::hash_combine;
using ::boost::hash_range;
} // namespace boost

namespace cactus::ecs {

export using Entity = size_t;

} // namespace cactus::ecs
