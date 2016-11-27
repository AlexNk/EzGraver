#ifndef RATETRACKER_H
#define RATETRACKER_H

#include <QObject>

#include <list>

/*!
 * This class is responsible to track the rate of pixels engraved within a certain time.
 */
class RateTracker : public QObject {
    Q_OBJECT

public:
    /*!
     * Creates a new instance of the rate tracker with the given parent object.
     *
     * \param parent The parent object.
     */
    RateTracker(QObject* parent=NULL);

    /*!
     * Tracks how many bytes have been received since the last invocation.
     *
     * \param count The number of bytes received since the last invocation.
     */
    void bytesReceived(int count);

    /*!
     * Gets the average engraving rate at this time.
     *
     * \return The average rate.
     */
    double rate() const;

private:
    std::list<int> _history;
};

#endif // RATETRACKER_H
