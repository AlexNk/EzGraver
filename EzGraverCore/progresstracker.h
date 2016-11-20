#ifndef PROGRESSMANAGER_H
#define PROGRESSMANAGER_H

#include "ezgravercore_global.h"

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QByteArray>

#include <memory>
#include <functional>

class EZGRAVERCORESHARED_EXPORT ProgressTracker : public QObject {
    Q_OBJECT

public:
    /*! The number of bytes a pixel is represented within the image. */
    static int const ImageBytesPerPixel{3};

    /*! The number of bytes a pixel is representeded within a status update. */
    static int const StatusBytesPerPixel{5};

    /*! The delay between each progress update while erasing the EEPROM. */
    static int const EraseProgressDelay{500};

    /*!
     * Creates a new instance of the progress tracker.
     *
     * \param parent The parent of object of the EzGraver instance.
     */
    ProgressTracker(QObject* parent=NULL);

    /*!
     * Processes the event that the EEPROM is being erased.
     *
     * \param eraseTime The time required to erase the EEPROM.
     */
    void eraseEepromStarted(int eraseTime);

    /*!
     * Processes the image upload event and initializes the image
     * upload status tracker.
     *
     * \param image The image being uploaded.
     * \param bytes The total bytes being transmitted.
     */
    void imageUploadStarted(QImage const& image, int bytes);

    /*!
     * Processes the engraving start event.
     */
    void engravingStarted();

    /*!
     * Processes the event that the engraving process was resetted.
     */
    void engravingResetted();

    /*!
     * Processes the event that there was some engraving process.
     *
     * \param statusBytes The status bytes of the progress update.
     */
    void statusBytesReceived(QByteArray const& statusBytes);

    virtual ~ProgressTracker();

public slots:
    /*!
     * Processes the event that bytes have been written to the engraver.
     *
     * \param bytes The number of bytes that have been transferred.
     */
    void bytesWritten(qint64 bytes);

signals:
    /*!
     * This signal is emitted whenever the engraver made some progress or
     * the maximum value changed.
     *
     * \param progress The current progress.
     * \param maximum The maximum progress.
     */
    void engraveProgressChanged(int progress, int maximum);

    /*!
     * This signal is emitted whenever there was some progress
     * when uploading the image to the engraver.
     *
     * \param progress The current progress.
     * \param maximum The maximum progress.
     */
    void uploadProgressChanged(int progress, int maximum);

    /*!
     * This signal is emitted whenever there was some progress
     * when erasing the EEPROM.
     *
     * \param progress The current progress.
     * \param maximum The maximum progress.
     */
    void eraseEepromProgressChanged(int progress, int maximum);

    /*!
     * This signal is emitted as soon as the EEPROM was completely
     * erased.
     */
    void eraseEepromCompleted();

private:
    int _eraseProgress;
    int _eraseTime;
    int _uploadProgress;
    int _bytesToUpload;
    int _engraveProgress;
    int _bytesToEngrave;

    std::function<void(qint64)> _bytesWrittenProcessor;
    std::function<void(QByteArray const&)> _statusByteProcessor;

    void _setEraseProgress(int progress);

    void _updateUploadProgress(qint64 bytes);
    void _updateEraseProgress(QTimer* timer);

    void _setEngraveProgress(int progress);
    void _setUploadProgress(int progress);
    void _updateEngravingProgress(QByteArray const& statusBytes);
};

#endif // PROGRESSMANAGER_H
