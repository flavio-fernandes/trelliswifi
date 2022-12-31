// Ref: https://github.com/Toshik/TickerScheduler/blob/a2b434752c33f389738dd602cf21ea3302f45275/src/TickerScheduler.cpp

#include "tickerScheduler.h"

void setExpired(bool *expiredPtr)
{
    *expiredPtr = true;
}

TsTicker::~TsTicker()
{
    ticker.detach();
}

void TsTicker::start()
{
    ticker.attach_ms(this->timer_milliseconds, setExpired, &this->expired);
}

void TsTicker::update()
{
    if (this->expired)
    {
        this->expired = false;
        this->callback();
    }
}

TickerScheduler::TickerScheduler() : tickers()
{
}

TickerScheduler::~TickerScheduler()
{
    for (auto &tickerPtr : tickers)
    {
        delete tickerPtr;
    }
    tickers.clear();
}

TsTicker &TickerScheduler::add(callback_t callback, uint32_t timer_milliseconds)
{
    auto tsTickerPtr = new TsTicker(callback, timer_milliseconds);
    tickers.push_back(tsTickerPtr);
    return *tsTickerPtr;
}

void TickerScheduler::update()
{
    for (auto &tickerPtr : tickers)
    {
        tickerPtr->update();
    }
}
