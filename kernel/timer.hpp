#pragma once

#include <cstdint>
#include <queue>
#include <vector>
#include <limits>
#include "message.hpp"

// deque is not queue. it have push_front(), push_back(), pop_front(), pop_back().
// use only push_back() and pop_front() to represent FIFO.
void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

class Timer {
	public:
		Timer(unsigned long timeout, int value, uint64_t task_id);
		unsigned long Timeout() const { return timeout_; }
		int Value() const { return value_; }
		uint64_t TaskID() const { return task_id_; }

	private:
		unsigned long timeout_;
		int value_;
		uint64_t task_id_;
};

// compare priority in priority_queue. more far timeout, lower priority.
inline bool operator<(const Timer& lhs, const Timer& rhs) {
	return lhs.Timeout() > rhs.Timeout();
}

class TimerManager {
	public:
		TimerManager();
		void AddTimer(const Timer& timer);
		bool Tick();
		unsigned long CurrentTick() const { return tick_; }

	private:
		// compiler of c++ doesn't recognize interrupt.
		volatile unsigned long tick_{0};
		std::priority_queue<Timer> timers_{};
};

extern TimerManager* timer_manager;
// count number per 1 seconds.
extern unsigned long lapic_timer_freq;
// frequency of Tick() per 1 seconds;
const int kTimerFreq = 100;

const int kTaskTimerPeriod = static_cast<int>(kTimerFreq * 0.02);
const int kTaskTimerValue = std::numeric_limits<int>::max();
