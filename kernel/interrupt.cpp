#include "interrupt.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "timer.hpp"

std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(InterruptDescriptor& desc,
		InterruptDescriptorAttribute attr,
		uint64_t offset,
		uint16_t segment_selector) {
	desc.attr = attr;
	desc.offset_low = offset & 0xffffu;
	desc.offset_middle = (offset >> 16) & 0xffffu;
	desc.offset_high = offset >> 32;
	desc.segment_selector = segment_selector;
}

void NotifyEndOfInterrupt() {
	volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
	*end_of_interrupt = 0;
}

namespace {
	// std::deque is doubie-ended queue. can access by index.
	// can use push_front, push_back(), pop_front(), pop_back().
	// to use as queue, use push_back() and pop_front()
	// to use as stack, use push_back() and pop_back()
	// std::queue use std::deque inside, in default.
	std::deque<Message>* msg_queue;

	__attribute__((interrupt))
	void IntHandlerXHCI(InterruptFrame* frame) {
		// Message{Message::kInterruptXHCI} is initializer list. Message struct has only one field, so we can initialize with this code.
		msg_queue->push_back(Message{Message::kInterruptXHCI});
		NotifyEndOfInterrupt();
	}

	__attribute__((interrupt))
	void IntHandlerLAPICTimer(InterruptFrame* frame) {
		LAPICTimerOnInterrupt();
		NotifyEndOfInterrupt();
	}
}

void InitializeInterrupt(std::deque<Message>* msg_queue) {
	// :: represent global scope. so global scope msg_queue is assigned msg_queue of this parameter.
	::msg_queue = msg_queue;

	SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
		    reinterpret_cast<uint64_t>(IntHandlerXHCI), kKernelCS);
	SetIDTEntry(idt[InterruptVector::kLAPICTimer], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
		    reinterpret_cast<uint64_t>(IntHandlerLAPICTimer), kKernelCS);

	
	LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}
