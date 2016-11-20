#include "progresstracker.h"

#include <QDebug>

ProgressTracker::ProgressTracker(QObject* parent) : QObject{parent}, _engraveProgress{}, _bytesToEngrave{},
        _uploadProgress{}, _bytesToUpload{}, _bytesWrittenProcessor{[](qint64){}}, _eraseProgress{}, _eraseTime{} {
}

ProgressTracker::~ProgressTracker() {}

bool _updateProgressGuarded(int newProgress, int& progress, int const& maximum) {
    if(maximum == 0) {
        return false;
    }

    if(newProgress > maximum) {
        progress = maximum;
    } else {
        progress = newProgress;
    }

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

void ProgressTracker::updateEngravingProgress(QByteArray const& statusBytes) {
    qDebug() << "received " << statusBytes.size() << " bytes.";
    _setEngraveProgress(_engraveProgress + statusBytes.size());
}

void ProgressTracker::imageUploadStarted(QImage const& image, int bytes) {
    auto bits = image.bits();
    _bytesWrittenProcessor = std::bind(&ProgressTracker::_updateUploadProgress, this, std::placeholders::_1);
    _bytesToUpload = bytes;
    _bytesToEngrave = std::count(bits, bits+image.byteCount(), 0x00) / ImageBytesPerPixel * StatusBytesPerPixel;
    qDebug() << "image has " << _bytesToEngrave << " bytes to engrave";

    _setUploadProgress(0);
}

void ProgressTracker::eraseEepromStarted(int eraseTime) {
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

void ProgressTracker::engravingResetted() {
    _setEngraveProgress(0);
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

