/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <cstdint>

#include "hal_defs.h"
#include "modbus_rtu_master.h"

#include "Timer.h"
#include "Record.h"
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
	struct _init_s    { void operator()(void); };
	struct _idle_s    { void operator()(void); };
	struct _request_s { void operator()(void); };
	struct _wait_s    { void operator()(void); };
	struct _save_s    { void operator()(void); };

	FSM_CREATE_STATE(init_s, _init_s);
	FSM_CREATE_STATE(idle_s, _idle_s);
	FSM_CREATE_STATE(request_s, _request_s);
	FSM_CREATE_STATE(wait_s, _wait_s);
	FSM_CREATE_STATE(save_s, _save_s);

	// Actions:
	struct none_a           { void operator()(void); };
	struct init_sens_a      { void operator()(void); };
	struct wait_start_a     { void operator()(void); };
	struct save_start_a     { void operator()(void); };
	struct idle_start_a     { void operator()(void); };
	struct iterate_sens_a   { void operator()(void); };
	struct count_error_a    { void operator()(void); };
	struct register_error_a { void operator()(void); };

	// FSM table:
	using fsm_table = fsm::TransitionTable<
		fsm::Transition<init_s,    ready_e,        idle_s,    none_a,           fsm::Guard::NO_GUARD>,

		fsm::Transition<idle_s,    need_measure_e, request_s, init_sens_a,      fsm::Guard::NO_GUARD>,

		fsm::Transition<request_s, sended_e,       wait_s,    wait_start_a,     fsm::Guard::NO_GUARD>,
		fsm::Transition<request_s, sens_end_e,     save_s,    save_start_a,     fsm::Guard::NO_GUARD>,
		fsm::Transition<request_s, no_sens_e,      idle_s,    idle_start_a,     fsm::Guard::NO_GUARD>,
		fsm::Transition<request_s, skip_e,         request_s, iterate_sens_a,   fsm::Guard::NO_GUARD>,

		fsm::Transition<wait_s,    response_e,     request_s, iterate_sens_a,   fsm::Guard::NO_GUARD>,
		fsm::Transition<wait_s,    timeout_e,      request_s, count_error_a,    fsm::Guard::NO_GUARD>,
		fsm::Transition<wait_s,    error_e,        request_s, iterate_sens_a,   fsm::Guard::NO_GUARD>,

		fsm::Transition<save_s,    saved_e,        idle_s,    idle_start_a,     fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,    timeout_e,      save_s,    count_error_a,    fsm::Guard::NO_GUARD>,
		fsm::Transition<save_s,    error_e,        idle_s,    register_error_a, fsm::Guard::NO_GUARD>
	>;

	static void response_packet_handler (modbus_response_t*);

protected:
	static uint32_t sensIndex;
	static uint8_t  errorsCount;

	static constexpr char TAG[] = "MSR";
	static constexpr uint8_t ERRORS_MAX = 10;

	static fsm::FiniteStateMachine<fsm_table> fsm;
	static utl::Timer timer;

public:
	static Record record;

	Measure();
	void process();

};
