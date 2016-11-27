#include "ratetracker.h"

#include <algorithm>

namespace {
static int const RequiredHistory{3};
static int const Weights[3] = {1, 2, 1};
static int const TotalWeight{4};
}

RateTracker::RateTracker(QObject* parent) : QObject{parent}, _history{} {

}

void RateTracker::bytesReceived(int count) {
    _history.push_back(count);
    if(_history.size() > RequiredHistory) {
        _history.pop_front();
    }
}

double RateTracker::rate() const {
    if(_history.size() < RequiredHistory) {
        return 0;
    }

    std::list<double> weighted{};
    std::transform(_history.cbegin(), _history.cend(), Weights, std::back_inserter(weighted), std::multiplies<int>());
    return std::accumulate(weighted.cbegin(), weighted.cend(), 0, std::plus<int>()) / TotalWeight;
}
