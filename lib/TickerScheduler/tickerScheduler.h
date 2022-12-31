
// Ref: https://github.com/Toshik/TickerScheduler/blob/a2b434752c33f389738dd602cf21ea3302f45275/src/TickerScheduler.h

#ifndef TICKERSCHEDULER_H
#define TICKERSCHEDULER_H

#include <inttypes.h>
#include <list>
#include <Ticker.h>

typedef void (*callback_t)();

class TsTicker
{
private:
	callback_t callback;
	uint32_t timer_milliseconds;
	Ticker ticker;
	bool expired;

	TsTicker() = delete;
	TsTicker(const TsTicker &other) = delete;
	TsTicker &operator=(const TsTicker &other) = delete;

public:
	TsTicker(callback_t callback, uint32_t timer_milliseconds) : callback(callback), timer_milliseconds(timer_milliseconds), ticker(), expired(false) {}
	~TsTicker();

	void start();
	void update();
};

class TickerScheduler
{
private:
	typedef std::list<TsTicker *> TsTickers;
	TsTickers tickers;

	TickerScheduler(const TickerScheduler &other) = delete;
	TickerScheduler &operator=(const TickerScheduler &other) = delete;

public:
	TickerScheduler();
	~TickerScheduler();

	TsTicker &add(callback_t callback, uint32_t timer_milliseconds);
	inline void sched(callback_t callback, uint32_t timer_milliseconds)
	{
		add(callback, timer_milliseconds).start();
	}
	void update();
};

#endif
