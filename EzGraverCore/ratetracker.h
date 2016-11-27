#ifndef RATETRACKER_H
#define RATETRACKER_H

#include <QObject>

#include <list>

class RateTracker : public QObject {
    Q_OBJECT

public:
    RateTracker(QObject* parent=NULL);

    void bytesReceived(int count);
    double rate() const;

private:
    std::list<int> _history;
};

#endif // RATETRACKER_H
