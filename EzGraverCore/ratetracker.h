#ifndef RATETRACKER_H
#define RATETRACKER_H

#include <QObject>
#include <QElapsedTimer>

#include <list>

/*!
 * This class is responsible to track the rate of pixels engraved within a certain time.
 */
class RateTracker : public QObject {
    Q_OBJECT

    Q_PROPERTY(double rate READ rate NOTIFY rateChanged)

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

signals:
    /*!
     * Emitted as soon as the transfer rate changed.
     *
     * \param rate The new transfer rate.
     */
    void rateChanged(double rate);

private:
    QElapsedTimer _timer;
    std::list<int> _history;
    double _rate;

    void _updateRate();
};

#endif // RATETRACKER_H
