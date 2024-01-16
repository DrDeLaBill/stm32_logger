/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


struct StackWatchdog
{
	void check();

private:
	static constexpr char TAG[] = "STCK";
	static unsigned lastFree;
};
