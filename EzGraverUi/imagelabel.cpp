#include "imagelabel.h"

#include <QPainter>
#include <QDebug>

#include <algorithm>

#include "ezgraver.h"

ImageLabel::ImageLabel(QWidget* parent)
    : ClickLabel{parent}
    , _image{}
    , _layerBurn{QSize{EzGraver::ImageWidth, EzGraver::ImageHeight}, QImage::Format_ARGB32}
    , _flags{Qt::DiffuseDither}
    , _grayscale{false}
    , _layer{0}
    , _layerCount{3}
    , _keepAspectRatio{false}
{   }

ImageLabel::~ImageLabel() {}

QImage ImageLabel::image() const {
    return _image;
}

void ImageLabel::setImage(QImage const& image) {
    _image = image;
    _burnCount = _burnedCount = 0;
    updateDisplayedImage();
    emit imageLoadedChanged(true);
    emit imageChanged(image);
}

Qt::ImageConversionFlags ImageLabel::conversionFlags() const {
    return _flags;
}

void ImageLabel::setConversionFlags(Qt::ImageConversionFlags const& flags) {
    _flags = flags;
    updateDisplayedImage();
    emit conversionFlagsChanged(flags);
}

bool ImageLabel::grayscale() const {
    return _grayscale;
}

void ImageLabel::setGrayscale(bool const& enabled) {
    _grayscale = enabled;
    updateDisplayedImage();
    emit grayscaleChanged(enabled);
}

int ImageLabel::layer() const {
    return _layer;
}

void ImageLabel::setLayer(int const& layer) {
    _layer = layer;
    updateDisplayedImage();
    emit layerChanged(layer);
}

int ImageLabel::layerCount() const {
    return _layerCount;
}

void ImageLabel::setLayerCount(int const& layerCount) {
    _layerCount = layerCount;
    updateDisplayedImage();
    emit layerCountChanged(layerCount);
}

bool ImageLabel::keepAspectRatio() const {
    return _keepAspectRatio;
}

void ImageLabel::setKeepAspectRatio(bool const& keepAspectRatio) {
    _keepAspectRatio = keepAspectRatio;
    updateDisplayedImage();
    emit keepAspectRatioChanged(keepAspectRatio);
}

void ImageLabel::updateDisplayedImage() {
    if(!imageLoaded()) {
        return;
    }

    // Draw white background, otherwise transparency is converted to black.
    QImage image{QSize{EzGraver::ImageWidth, EzGraver::ImageHeight}, QImage::Format_ARGB32};
    image.fill(QColor{Qt::white});
    QPainter painter{&image};

    // As at this time, the target image is quadratic, scaling according the larger dimension is sufficient.
    auto scaled = _keepAspectRatio
              ? (_image.width() > _image.height() ? _image.scaledToWidth(image.width()) : _image.scaledToHeight(image.height()))
              : _image.scaled(image.size());
    auto position = _keepAspectRatio
            ? (_image.width() > _image.height() ? QPoint(0, (image.height() - scaled.height()) / 2) : QPoint((image.width() - scaled.width()) / 2, 0))
            : QPoint(0, 0);
    painter.drawImage(position, scaled);

    _displayImg = _grayscale ? _createGrayscaleImage(image) : image.convertToFormat(QImage::Format_Mono, _flags);
    updateInfoLayers();
}

QImage ImageLabel::_createGrayscaleImage(QImage const& original) const {
    auto colorTable = _createColorTable();
    QImage grayed = original.convertToFormat(QImage::Format_Indexed8, colorTable, _flags);
    if(_layer == 0) {
        return grayed;
    }

    auto visibleLayer = _layer-1;
    int i{0};
    std::transform(colorTable.begin(), colorTable.end(), colorTable.begin(), [&i,visibleLayer](QRgb) {
        return i++ == visibleLayer ? qRgb(0, 0, 0) : qRgb(255, 255, 255);
    });
    grayed.setColorTable(colorTable);

    return grayed.convertToFormat(QImage::Format_Mono, _flags);
}

QVector<QRgb> ImageLabel::_createColorTable() const {
    QVector<QRgb> colorTable(_layerCount - 1);

    int i{0};
    std::generate(colorTable.begin(), colorTable.end(), [this, &i] {
      int gray = (256 / (_layerCount-1)) * (i++);
      return qRgb(gray, gray, gray);
    });
    colorTable.push_back(qRgb(255, 255, 255));

    return colorTable;
}

bool ImageLabel::imageLoaded() const {
    return !_image.isNull();
}

void ImageLabel::setImageDimensions(QSize const& dimensions) {
    auto span = this->lineWidth()*2;
    setMinimumWidth(dimensions.width() + span);
    setMinimumHeight(dimensions.height() + span);
}

int ImageLabel::picX() const {
    return _picX0;
}

int ImageLabel::picY() const {
    return _picY0;
}

int ImageLabel::picW() const {
    return _picX1 - _picX0 + 1;
}

int ImageLabel::picH() const {
    return _picY1 - _picY0 + 1;
}

int ImageLabel::markBurnedPixel(int x, int y) {
    _layerBurn.setPixel(x, y, qRgba(0xFF, 0x00, 0x00, 0xFF));
    updateDisplayedImage();
    return ++_burnedCount;
}

void ImageLabel::resetBurnStatus() {
    _layerBurn.fill(qRgba(0, 0, 0, 0));
    _burnedCount = 0;
    updateDisplayedImage();
}

void ImageLabel::updateDimensions(QImage const & image) {
    // _picY0 = _picY1 = _picX0 = _picX1 = -1;
    auto const w = image.width();
    auto const h = image.height();
    _picX0 = w;
    _picY0 = h;
    _picX1 = 0;
    _picY1 = 0;
    _burnCount = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            if (image.pixel(x, y) == qRgba(0, 0, 0, 0xFF))
            {
                ++_burnCount;
                _picX0 = std::min(_picX0, x);
                _picY0 = std::min(_picY0, y);
                _picX1 = std::max(_picX1, x);
                _picY1 = std::max(_picY1, y);
            }
        }
}

void ImageLabel::updateInfoLayers() {
    auto img = _displayImg.convertToFormat(QImage::Format_ARGB32, 0);
    QPainter painter{&img};
    painter.drawImage(QPoint(0, 0), _layerBurn);

    auto rendered = QPixmap::fromImage(img);
    setPixmap(rendered);
    updateDimensions(img);
}