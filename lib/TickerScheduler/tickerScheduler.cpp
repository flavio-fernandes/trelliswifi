// Ref: https://github.com/Toshik/TickerScheduler/blob/a2b434752c33f389738dd602cf21ea3302f45275/src/TickerScheduler.cpp

#include "tickerScheduler.h"

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

Ticker &TickerScheduler::add(fptr callback, uint32_t timer, uint32_t repeat, resolution_t resolution)
{
    auto tickerPtr = new Ticker(callback, timer, repeat, resolution);
    tickers.push_back(tickerPtr);
    return *tickerPtr;
}

void TickerScheduler::update()
{
    for (auto &tickerPtr : tickers)
    {
        tickerPtr->update();
    }
}
