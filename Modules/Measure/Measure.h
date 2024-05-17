/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <cstdint>

#include "hal_defs.h"
#include "modbus_rtu_master.h"

#include "Timer.h"
#include "RecordDB.h"
#include "FiniteStateMachine.h"


class Measure
{
protected:
	// Events:
	FSM_CREATE_EVENT(success_e,      0);
	FSM_CREATE_EVENT(timeout_e,      1);
	FSM_CREATE_EVENT(iterate_e,      2);
	FSM_CREATE_EVENT(end_e,          3);
	FSM_CREATE_EVENT(error_e,        4);

private:
	// States:
	struct _idle_s        { void operator()(void); };
	struct _wait_start_s  { void operator()(void); };
	struct _mb1_request_s { void operator()(void); };
	struct _mb1_wait_s    { void operator()(void); };
	struct __1w_delay_s   { void operator()(void); };
	struct __1w_request_s { void operator()(void); };
	struct __1w_wait_s    { void operator()(void); };
	struct _save_s        { void operator()(void); };

	FSM_CREATE_STATE(idle_s,        _idle_s);
	FSM_CREATE_STATE(wait_start_s,  _wait_start_s);
	FSM_CREATE_STATE(mb1_request_s, _mb1_request_s);
	FSM_CREATE_STATE(mb1_wait_s,    _mb1_wait_s);
	FSM_CREATE_STATE(_1w_delay_s,   __1w_delay_s);
	FSM_CREATE_STATE(_1w_request_s, __1w_request_s);
	FSM_CREATE_STATE(_1w_wait_s,    __1w_wait_s);
	FSM_CREATE_STATE(save_s,        _save_s);

	// Actions:
	struct wait_start_a       { void operator()(void); };
	struct wait_response_a    { void operator()(void); };
	struct save_start_a       { void operator()(void); };
	struct idle_start_a       { void operator()(void); };
	struct init_mb1_sens_a    { void operator()(void); };
	struct init_1w_sens_a     { void operator()(void); };
	struct iterate_mb1_sens_a { void operator()(void); };
	struct iterate_1w_sens_a  { void operator()(void); };
	struct count_error_a      { void operator()(void); };
	struct register_error_a   { void operator()(void); };
	struct start_delay_a      { void operator()(void); };

	// FSM table:
	using fsm_table = fsm::TransitionTable<
		fsm::Transition<idle_s,        success_e, wait_start_s,  wait_start_a,       fsm::Guard::NO_GUARD>,

		fsm::Transition<wait_start_s,  timeout_e,      mb1_request_s, init_mb1_sens_a,    fsm::Guard::NO_GUARD>,

		fsm::Transition<mb1_request_s, success_e,      mb1_wait_s,    wait_response_a,    fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_request_s, end_e,          _1w_delay_s,   start_delay_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_request_s, iterate_e,      mb1_request_s, iterate_mb1_sens_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_wait_s,    iterate_e,      mb1_request_s, iterate_mb1_sens_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_wait_s,    timeout_e,      mb1_request_s, count_error_a,      fsm::Guard::NO_GUARD>,

		fsm::Transition<_1w_delay_s,   success_e,      _1w_request_s, init_1w_sens_a,     fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_delay_s,   timeout_e,      save_s,        save_start_a,       fsm::Guard::NO_GUARD>,

		fsm::Transition<_1w_request_s, success_e,      _1w_wait_s,    wait_response_a,    fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_request_s, end_e,          save_s,        save_start_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_request_s, iterate_e,      _1w_request_s, iterate_1w_sens_a,  fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_wait_s,    iterate_e,      _1w_request_s, iterate_1w_sens_a,  fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_wait_s,    timeout_e,      _1w_request_s, count_error_a,      fsm::Guard::NO_GUARD>,

		fsm::Transition<save_s,        success_e,      idle_s,        idle_start_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,        timeout_e,      save_s,        count_error_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,        error_e,        idle_s,        register_error_a,   fsm::Guard::NO_GUARD>
	>;

	static void response_packet_handler (modbus_response_t*);

protected:
	static uint8_t sensAddress;
	static uint8_t sensIdx;
	static uint8_t errorsCount;

	static constexpr char TAG[] = "MSR";
	static constexpr uint8_t ERRORS_MAX = 10;

	static fsm::FiniteStateMachine<fsm_table> fsm;
	static utl::Timer timer;

public:
	static RecordDB record;

	Measure();
	void process();

};
