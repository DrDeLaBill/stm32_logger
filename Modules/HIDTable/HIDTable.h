/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _USB_HID_TABLE_H_
#define _USB_HID_TABLE_H_


#include <cstdint>
#include <variant>

#include "HIDHash.h"
#include "HIDTuple.h"
#include "settings.h"
#include "TypeListService.h"
#include "TypeListBuilder.h"

struct HIDTableBase {};

template<class... TuplesTable>
struct HIDTable : HIDTableBase
{
    static_assert(
        !utl::empty(typename utl::typelist_t<TuplesTable...>::RESULT{}),
        "HID empty tuples table"
    );

    static_assert(
        std::is_same_v<
            typename utl::variant_factory<utl::typelist_t<TuplesTable...>>::VARIANT,
            typename utl::variant_factory<utl::removed_duplicates_t<TuplesTable...>>::VARIANT
            >,
        "HID repeated tuples"
    );

    using tuple_p = utl::simple_list_t<TuplesTable...>;
    using tuple_v = std::variant<TuplesTable...>;
};


#endif
