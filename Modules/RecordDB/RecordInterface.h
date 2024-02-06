#ifndef _RECORD_INTERFACE_H_
#define _RECORD_INTERFACE_H_


#include "Measurer.h"


struct RecordInterface
{
	// TODO: load PC needed record (not last in Measurer)
    struct id     { uint32_t* operator()() { return &(Measurer::record.record.id); } };
    struct time   { uint32_t* operator()() { return &(Measurer::record.record.time); } };
//    struct IDs    { uint8_t*  operator()() { return record.IDs; } };
//    struct values { uint16_t* operator()() { return record.values; } }; // TODO: ids and values for HID
};


#endif
