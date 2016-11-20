#include "progresstracker.h"

#include <QDebug>

ProgressTracker::ProgressTracker(QObject* parent) : QObject{parent}, _engraveProgress{}, _bytesToEngrave{},
        _uploadProgress{}, _bytesToUpload{}, _bytesWrittenProcessor{[](qint64){}}, _eraseProgress{}, _eraseTime{} {
}

ProgressTracker::~ProgressTracker() {}

void ProgressTracker::_setEngraveProgress(int progress) {
    if(_bytesToEngrave > 0) {
        _engraveProgress = progress;
        emit engraveProgressChanged(_engraveProgress, _bytesToEngrave);
    }
}

void ProgressTracker::_setUploadProgress(int progress) {
    if(_bytesToUpload > 0) {
        _uploadProgress = progress;
        emit uploadProgressChanged(_uploadProgress, _bytesToUpload);
    }
}

void ProgressTracker::_setEraseProgress(int progress) {
    _eraseProgress = progress;
    emit eraseEepromProgressChanged(_eraseProgress, _eraseTime);
}

void ProgressTracker::updateEngravingProgress(QByteArray const& statusBytes) {
    qDebug() << "received " << statusBytes.size() << " bytes.";
    _setEngraveProgress(statusBytes.size());
}

void ProgressTracker::imageUploadStarted(QImage const& image, int bytes) {
    auto bits = image.bits();
    _bytesWrittenProcessor = std::bind(&ProgressTracker::_updateUploadProgress, this, std::placeholders::_1);
    _bytesToUpload = bytes;
    _bytesToEngrave = std::count_if(bits, bits+image.byteCount(), [](uchar byte) { return byte == 0xFF; }) / ImageBytesPerPixel * StatusBytesPerPixel;
}

void ProgressTracker::eraseEepromStarted(int eraseTime) {
    _eraseTime = eraseTime;
    QTimer* timer{new QTimer{this}};
    connect(timer, &QTimer::timeout, std::bind(&ProgressTracker::_updateEraseProgress, this, timer));
    timer->start(EraseProgressDelay);
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

