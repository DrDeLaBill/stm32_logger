/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <cstdint>

#include "hal_defs.h"
#include "modbus_rtu_master.h"

#include "Timer.h"
#include "RecordDB.h"
#include "FiniteStateMachine.h"


#define MEASURER_BEDUG (false)


class Measure
{
protected:
	// Events:
	FSM_CREATE_EVENT(ready_e,        0);
	FSM_CREATE_EVENT(timeout_e,      0);
	FSM_CREATE_EVENT(sended_e,       0);
	FSM_CREATE_EVENT(saved_e,        0);
	FSM_CREATE_EVENT(need_measure_e, 0);
	FSM_CREATE_EVENT(response_e,     1);
	FSM_CREATE_EVENT(skip_e,         1);
	FSM_CREATE_EVENT(sens_end_e,     2);
	FSM_CREATE_EVENT(no_sens_e,      3);
	FSM_CREATE_EVENT(error_e,        4);

private:
	// States:
	struct _init_s        { void operator()(void); };
	struct _idle_s        { void operator()(void); };
	struct _mb1_request_s { void operator()(void); };
	struct _mb1_wait_s    { void operator()(void); };
	struct __1w_request_s { void operator()(void); };
	struct __1w_wait_s    { void operator()(void); };
	struct _save_s        { void operator()(void); };

	FSM_CREATE_STATE(init_s,        _init_s);
	FSM_CREATE_STATE(idle_s,        _idle_s);
	FSM_CREATE_STATE(mb1_request_s, _mb1_request_s);
	FSM_CREATE_STATE(mb1_wait_s,    _mb1_wait_s);
	FSM_CREATE_STATE(_1w_request_s, __1w_request_s);
	FSM_CREATE_STATE(_1w_wait_s,    __1w_wait_s);
	FSM_CREATE_STATE(save_s,        _save_s);

	// Actions:
	struct none_a             { void operator()(void); };
	struct init_sens_a        { void operator()(void); };
	struct wait_start_a       { void operator()(void); };
	struct save_start_a       { void operator()(void); };
	struct idle_start_a       { void operator()(void); };
	struct iterate_mb1_sens_a { void operator()(void); };
	struct iterate_1w_sens_a  { void operator()(void); };
	struct count_error_a      { void operator()(void); };
	struct register_error_a   { void operator()(void); };

	// FSM table:
	using fsm_table = fsm::TransitionTable<
		fsm::Transition<init_s,        ready_e,        idle_s,        none_a,             fsm::Guard::NO_GUARD>,

		fsm::Transition<idle_s,        need_measure_e, mb1_request_s, init_sens_a,        fsm::Guard::NO_GUARD>,

		fsm::Transition<mb1_request_s, sended_e,       mb1_wait_s,    wait_start_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_request_s, sens_end_e,     _1w_request_s, init_sens_a,        fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_request_s, no_sens_e,      _1w_request_s, init_sens_a,        fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_request_s, skip_e,         mb1_request_s, iterate_mb1_sens_a, fsm::Guard::NO_GUARD>,

		fsm::Transition<mb1_wait_s,    response_e,     mb1_request_s, iterate_mb1_sens_a, fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_wait_s,    timeout_e,      mb1_request_s, count_error_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<mb1_wait_s,    error_e,        mb1_request_s, iterate_mb1_sens_a, fsm::Guard::NO_GUARD>,

		fsm::Transition<_1w_request_s, sended_e,       _1w_wait_s,    wait_start_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_request_s, sens_end_e,     save_s,        save_start_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_request_s, no_sens_e,      save_s,        idle_start_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_request_s, skip_e,         _1w_request_s, iterate_1w_sens_a,  fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_wait_s,    response_e,     _1w_request_s, iterate_1w_sens_a,  fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_wait_s,    timeout_e,      _1w_request_s, count_error_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<_1w_wait_s,    error_e,        _1w_request_s, iterate_1w_sens_a,  fsm::Guard::NO_GUARD>,

		fsm::Transition<save_s,       saved_e,        idle_s,         idle_start_a,       fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,       timeout_e,      save_s,         count_error_a,      fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,       error_e,        idle_s,         register_error_a,   fsm::Guard::NO_GUARD>
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
