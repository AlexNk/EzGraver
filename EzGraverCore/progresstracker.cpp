#include "progresstracker.h"

#include <QDebug>

ProgressTracker::ProgressTracker(QObject* parent) : QObject{parent},
        _eraseProgress{}, _eraseTime{}, _uploadProgress{}, _bytesToUpload{}, _engraveProgress{}, _bytesToEngrave{},
        _bytesWrittenProcessor{[](qint64){}}, _statusByteProcessor{[](QByteArray const&){}} {
}

ProgressTracker::~ProgressTracker() {}

bool _updateProgressGuarded(int newProgress, int& progress, int const& maximum) {
    if(maximum == 0) {
        return false;
    }

    progress = (newProgress > maximum) ? maximum : newProgress;
    return true;
}

void ProgressTracker::_setEngraveProgress(int progress) {
    if(_updateProgressGuarded(progress, _engraveProgress, _bytesToEngrave)) {
        emit engraveProgressChanged(_engraveProgress, _bytesToEngrave);
    }
}

void ProgressTracker::_setUploadProgress(int progress) {
    if(_updateProgressGuarded(progress, _uploadProgress, _bytesToUpload)) {
        emit uploadProgressChanged(_uploadProgress, _bytesToUpload);
    }
}

void ProgressTracker::_setEraseProgress(int progress) {
    if(_updateProgressGuarded(progress, _eraseProgress, _eraseTime)) {
        emit eraseEepromProgressChanged(_eraseProgress, _eraseTime);
    }
}

void ProgressTracker::statusBytesReceived(QByteArray const& statusBytes) {
    qDebug() << "received" << statusBytes.size() << "bytes:" << statusBytes;
    _statusByteProcessor(statusBytes);
}

void ProgressTracker::_updateEngravingProgress(QByteArray const& statusBytes) {
    _setEngraveProgress(_engraveProgress + statusBytes.size());
    if(_engraveProgress >= _bytesToEngrave) {
        _statusByteProcessor = [](QByteArray const&){};
    }
}

void ProgressTracker::imageUploadStarted(QImage const& image, int bytes) {
    _bytesWrittenProcessor = std::bind(&ProgressTracker::_updateUploadProgress, this, std::placeholders::_1);
    _bytesToUpload = bytes;
    _setUploadProgress(0);

    auto bits = image.bits();
    _bytesToEngrave = std::count(bits, bits+image.byteCount(), 0x00) / ImageBytesPerPixel * StatusBytesPerPixel;
    qDebug() << "image has" << _bytesToEngrave << "bytes to engrave";
}

void ProgressTracker::eraseEepromStarted(int eraseTime) {
    engravingResetted();

    _eraseTime = eraseTime;
    QTimer* timer{new QTimer{this}};
    connect(timer, &QTimer::timeout, std::bind(&ProgressTracker::_updateEraseProgress, this, timer));
    timer->start(EraseProgressDelay);

    _setEraseProgress(0);
}

void ProgressTracker::_updateEraseProgress(QTimer* timer) {
    auto progress = _eraseProgress + EraseProgressDelay;
    _setEraseProgress(progress);
    if(progress >= _eraseTime) {
        timer->stop();
        emit eraseEepromCompleted();
    }
}

void ProgressTracker::engravingStarted() {
    _statusByteProcessor = std::bind(&ProgressTracker::_updateEngravingProgress, this, std::placeholders::_1);
}

void ProgressTracker::engravingResetted() {
    _setEngraveProgress(0);
    _statusByteProcessor = [](QByteArray const&){};
}

void ProgressTracker::_updateUploadProgress(qint64 bytes) {
    qDebug() << "image bytes written:" << bytes;
    _setUploadProgress(_uploadProgress + bytes);
    if(_uploadProgress >= _bytesToUpload) {
        qDebug() << "image upload completed";
        _bytesWrittenProcessor = [](qint64){};
    }
}

void ProgressTracker::bytesWritten(qint64 bytes) {
    _bytesWrittenProcessor(bytes);
}

