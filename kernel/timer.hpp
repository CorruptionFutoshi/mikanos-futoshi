#pragma once

#include <cstdint>
#include <queue>
#include <vector>
#include "message.hpp"

// deque is not queue. it have push_front(), push_back(), pop_front(), pop_back().
// use only push_back() and pop_front() to represent FIFO.
void InitializeLAPICTimer(std::deque<Message>& msg_queue);
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

class Timer {
	public:
		Timer(unsigned long timeout, int value);
		unsigned long Timeout() const { return timeout_; }
		int Value() const { return value_; }

	private:
		unsigned long timeout_;
		int value_;
};

// compare priority in priority_queue. more far timeout, lower priority.
inline bool operator<(const Timer& lhs, const Timer& rhs) {
	return lhs.Timeout() > rhs.Timeout();
}

class TimerManager {
	public:
		TimerManager(std::deque<Message>& msg_queue);
		void AddTimer(const Timer& timer);
		void Tick();
		unsigned long CurrentTick() const { return tick_; }

	private:
		// compiler of c++ doesn't recognize interrupt.
		volatile unsigned long tick_{0};
		std::priority_queue<Timer> timers_{};
		std::deque<Message>& msg_queue_;
};

extern TimerManager* timer_manager;

void LAPICTimerOnInterrupt();
