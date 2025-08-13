module;

#include "tuplet/tuple.hpp"

export module tuplet;

export namespace tuplet {
using tuplet::type_list;

using tuplet::type_map;

using tuplet::tuple_elem;

using tuplet::tuple_base_t;

using tuplet::tuple;

using tuplet::pair;

using tuplet::convert;

using tuplet::apply;
using tuplet::forward_as_tuple;
using tuplet::get;
using tuplet::make_tuple;
using tuplet::swap;
using tuplet::tie;
using tuplet::tuple_cat;

namespace literals {
using tuplet::literals::operator""_tag;
}

} // namespace tuplet

export using std::tuple_size;
export using std::tuple_element;
