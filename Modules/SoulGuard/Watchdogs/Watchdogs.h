/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <cstdint>

#include "hal_defs.h"

#include "Timer.h"
#include "FiniteStateMachine.h"


#define SETTINGS_WATCHDOG_BEDUG (true)
#define POWER_WATCHDOG_BEDUG    (true)


#define WATCHDOG_TIMEOUT_MS     ((uint32_t)100)


/*
 * Filling an empty area of RAM with the STACK_CANARY_WORD value
 * For calculating the RAM fill factor
 */
extern "C" void STACK_WATCHDOG_FILL_RAM(void);


struct StackWatchdog
{
private:
	static constexpr char TAG[] = "STCK";
	static unsigned lastFree;
	static utl::Timer timer;

public:
	void check();

};

struct RestartWatchdog
{
public:
	// TODO: check IWDG or another reboot
	void check();

#ifdef EEPROM_I2C
	static void reset_i2c_errata();
#endif

private:
	static constexpr char TAG[] = "RSTw";
	static bool flagsCleared;

};

struct RTCWatchdog
{
	static constexpr char TAG[] = "RTCw";

	void check();
};


struct SettingsWatchdog
{
	SettingsWatchdog();

	void check();
};


struct MemoryWatchdog
{
private:
	static constexpr uint32_t TIMEOUT_MS = 15000;

	utl::Timer errorTimer;
	utl::Timer timer;
	uint8_t errors;
	bool timerStarted;

public:
	MemoryWatchdog();

	void check();
};

struct StandbyWatchdog
{
private:
#ifdef DEBUG
	static constexpr uint32_t DELTA_SEC = MINUTE_MS / SECOND_MS;
#else
	static constexpr uint32_t DELTA_SEC = HOUR_MS / SECOND_MS;
#endif

public:
	static constexpr char TAG[] = "STBY";

	StandbyWatchdog();

	static void alarm();

	void check();

	static bool isAlarmReady();
	static void startRTCAlarm(uint32_t seconds = sleepTimeSec());

	static bool hasWokenUp();
	static void clearPWRFlags();

	static bool needEnterStandby();
	static void enterStandby();

	static uint32_t sleepTimeSec();
};

struct PowerWatchdog
{
	void check();
};

struct OneWireWatcher
{
protected:
	static constexpr uint32_t TIMEOUT_MS = MINUTE_MS;
	static constexpr uint32_t DELAY_MS   = 5 * SECOND_MS;

	// Events:
	FSM_CREATE_EVENT(start_e,    0);
	FSM_CREATE_EVENT(received_e, 0);
	FSM_CREATE_EVENT(next_e,     0);
	FSM_CREATE_EVENT(done_e,     1);
	FSM_CREATE_EVENT(timeout_e,  2);

	// States:
	struct _idle_s       { void operator()(); };
	struct _start_s      { void operator()(); };
	struct _registrate_s { void operator()(); };
	struct _end_s        { void operator()(); };

	FSM_CREATE_STATE(idle_s,       _idle_s);
	FSM_CREATE_STATE(start_s,      _start_s);
	FSM_CREATE_STATE(registrate_s, _registrate_s);
	FSM_CREATE_STATE(end_s,        _end_s);

	// Actions:
	struct timeout_a      { void operator()(); };
	struct done_a         { void operator()(); };
	struct enable_a       { void operator()(); };
	struct start_search_a { void operator()(); };
	struct next_search_a  { void operator()(); };

	using fsm_table = fsm::TransitionTable<
		fsm::Transition<idle_s,       start_e,   start_s,      enable_a,       fsm::Guard::NO_GUARD>,

		fsm::Transition<start_s,      done_e,    registrate_s, start_search_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<start_s,      timeout_e, idle_s,       timeout_a,      fsm::Guard::NO_GUARD>,

		fsm::Transition<registrate_s, next_e,    registrate_s, next_search_a,  fsm::Guard::NO_GUARD>,
		fsm::Transition<registrate_s, start_e,   registrate_s, start_search_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<registrate_s, timeout_e, idle_s,       timeout_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<registrate_s, done_e,    end_s,        done_a,         fsm::Guard::NO_GUARD>,

		fsm::Transition<end_s,        done_e,    idle_s,       done_a,         fsm::Guard::NO_GUARD>
	>;

	static constexpr char TAG[] = "1WRE";

	static fsm::FiniteStateMachine<fsm_table> fsm;
	static uint8_t index;

	static utl::Timer timeoutTimer;
	static utl::Timer delayTimer;

public:
	void check();

};

struct SDCardWatcher
{
	static constexpr char TAG[] = "SDCw";
	static constexpr unsigned ERRORS_MAX = 7;

	unsigned errors;

	SDCardWatcher();
	void check();
};
