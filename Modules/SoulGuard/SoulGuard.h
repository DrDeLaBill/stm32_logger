/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <variant>

#include "settings.h"

#include "Watchdogs.h"
#include "TypeListBuilder.h"


template<class... Watchdogs>
struct SoulGuard
{
private:
	static_assert(
		std::is_same_v<
			typename utl::variant_factory<utl::typelist_t<Watchdogs...>>::VARIANT,
			typename utl::variant_factory<utl::removed_duplicates_t<Watchdogs...>>::VARIANT
		>,
		"Soul guard repeated units"
	);

	using watchdog_v = std::variant<Watchdogs...>;

	void check(watchdog_v watchdog)
	{
		auto lambda = [](auto& wtchdg) {
			wtchdg.check();
		};

		std::visit(lambda, watchdog);
	}

public:
	void defend()
	{
		(this->check(Watchdogs{}), ...);
	}

	bool hasErrors()
	{
		if (!is_settings_initialized()) {
			return true;
		}
		// TODO: errors list
		return false;
	}
};
