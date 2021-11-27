
// Ref: https://github.com/Toshik/TickerScheduler/blob/a2b434752c33f389738dd602cf21ea3302f45275/src/TickerScheduler.h

#ifndef TICKERSCHEDULER_H
#define TICKERSCHEDULER_H

#include <list>
#include <Ticker.h>

class TickerScheduler
{
private:
	typedef std::list<Ticker *> Tickers;
	Tickers tickers;

	TickerScheduler(const TickerScheduler &other) = delete;
	TickerScheduler &operator=(const TickerScheduler &other) = delete;

public:
	TickerScheduler();
	~TickerScheduler();

	Ticker &add(fptr callback, uint32_t timer, uint32_t repeat = 0, resolution_t resolution = MICROS);
	inline void sched(fptr callback, uint32_t timer_milliseconds)
	{
		add(callback, timer_milliseconds, 0, MILLIS).start();
	}
	void update();
};

#endif
