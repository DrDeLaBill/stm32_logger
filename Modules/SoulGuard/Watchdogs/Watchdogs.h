/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <cstdint>

#include "record.h"
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
protected:
	struct state_init   {void operator()(void) const;};
	struct state_idle   {void operator()(void) const;};
	struct state_save   {void operator()(void) const;};
	struct state_load   {void operator()(void) const;};

	struct action_check {void operator()(void) const;};

	FSM_CREATE_STATE(init_s, state_init);
	FSM_CREATE_STATE(idle_s, state_idle);
	FSM_CREATE_STATE(save_s, state_save);
	FSM_CREATE_STATE(load_s, state_load);

	FSM_CREATE_EVENT(saved_e,   0);
	FSM_CREATE_EVENT(updated_e, 0);

	using fsm_table = fsm::TransitionTable<
		fsm::Transition<init_s, updated_e,   idle_s, action_check, fsm::Guard::NO_GUARD>,

		fsm::Transition<idle_s, saved_e,     load_s, action_check, fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s, updated_e,   save_s, action_check, fsm::Guard::NO_GUARD>,

		fsm::Transition<load_s, updated_e,   idle_s, action_check, fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s, saved_e,     idle_s, action_check, fsm::Guard::NO_GUARD>
	>;

	static fsm::FiniteStateMachine<fsm_table> fsm;

private:
	static constexpr char TAG[] = "STGw";

public:
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

	static uint32_t sleepTimeSec();

protected:
	static utl::Timer timer;

	static bool isAlarmReady();
	static void startRTCAlarm(uint32_t seconds = sleepTimeSec());

	static bool hasWokenUp();
	static void clearPWRFlags();

	static bool needEnterStandby();
	static void enterStandby();


	// Events:
	FSM_CREATE_EVENT(loaded_e,             0);
	FSM_CREATE_EVENT(alarm_started_e,      0);
	FSM_CREATE_EVENT(started_e,            0);
	FSM_CREATE_EVENT(need_standby_e,       1);
	FSM_CREATE_EVENT(alarm_e,              2);
	FSM_CREATE_EVENT(need_restart_alarm_e, 2);
	FSM_CREATE_EVENT(need_start_alarm_e,   3);

	// States:
	struct _init_s  { void operator()(); };
	struct _idle_s  { void operator()(); };
	struct _start_s { void operator()(); };

	FSM_CREATE_STATE(init_s,  _init_s);
	FSM_CREATE_STATE(idle_s,  _idle_s);
	FSM_CREATE_STATE(start_s, _start_s);

	// Actions:
	struct check_last_alarm_a { void operator()(); };
	struct restart_alarm_a    { void operator()(); };
	struct start_alarm_a      { void operator()(); };
	struct enter_standby_a    { void operator()(); };
	struct check_alarm_a      { void operator()(); };

	using fsm_table = fsm::TransitionTable<
		fsm::Transition<init_s,  loaded_e,             idle_s,  check_last_alarm_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<init_s,  need_standby_e,       init_s,  enter_standby_a,    fsm::Guard::NO_GUARD>,

		fsm::Transition<idle_s,  need_restart_alarm_e, start_s, restart_alarm_a,    fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s,  need_start_alarm_e,   start_s, start_alarm_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s,  alarm_e,              start_s, start_alarm_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<idle_s,  need_standby_e,       idle_s,  enter_standby_a,    fsm::Guard::NO_GUARD>,

		fsm::Transition<start_s, started_e,            idle_s,  check_alarm_a,      fsm::Guard::NO_GUARD>
	>;

	static fsm::FiniteStateMachine<fsm_table> fsm;

public:
	static constexpr char TAG[] = "STBY";

	static void alarm();

	void check();
};

struct PowerWatchdog
{
private:
	static constexpr uint32_t REFERENSE_VOLTAGE = 1800; // U = 1.8 V
	static constexpr uint32_t VOLTAGE_MULTIPLIER = 2;
	static constexpr uint32_t TRIG_LEVEL_MIN = 2700; // U = 2.7V
	static constexpr uint32_t TRIG_LEVEL_MAX = 3600; // U = 2.7V
	static constexpr uint32_t TIMEOUT_MS = 100;
	static constexpr char TAG[] = "PWRw";

	static utl::Timer timer;

protected:
	// Events:
	FSM_CREATE_EVENT(success_e, 0);
	FSM_CREATE_EVENT(error_e,   1);

	// States:
	struct _init_s   { void operator()(); };
	struct _wait_s   { void operator()(); };
	struct _check_s  { void operator()(); };

	FSM_CREATE_STATE(init_s,  _init_s);
	FSM_CREATE_STATE(wait_s,  _wait_s);
	FSM_CREATE_STATE(check_s, _check_s);

	// Actions:
	struct start_DMA_a   { void operator()(); };
	struct check_power_a { void operator()(); };
	struct none_a        { void operator()(); };
	struct set_error_a   { void operator()(); };

	using fsm_table = fsm::TransitionTable<
		fsm::Transition<init_s,  success_e, wait_s,  start_DMA_a,   fsm::Guard::NO_GUARD>,

		fsm::Transition<wait_s,  success_e, check_s, check_power_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<wait_s,  error_e,   init_s,  none_a,        fsm::Guard::NO_GUARD>,

		fsm::Transition<check_s, success_e, wait_s,  start_DMA_a,   fsm::Guard::NO_GUARD>,
		fsm::Transition<check_s, error_e,   init_s,  set_error_a,   fsm::Guard::NO_GUARD>
	>;

	static fsm::FiniteStateMachine<fsm_table> fsm;


public:
	void check();

	static void stopDMA();
};

struct InfoWatchdog
{
private:
	RecordStatus loadMaxRecord();
	RecordStatus loadMinRecord();
	RecordStatus loadRecord();

public:
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
