#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QMimeData>
#include <QFileDialog>
#include <QBitmap>
#include <QIcon>
#include <QThreadPool>
#include <QDebug>

#include <stdexcept>

MainWindow::MainWindow(QWidget* parent)
        :  QMainWindow{parent}, _ui{new Ui::MainWindow},
          _portTimer{}, _ezGraver{}, _bytesWrittenProcessor{[](qint64){}}, _connected{false} {
    _ui->setupUi(this);
    setAcceptDrops(true);

    connect(&_portTimer, &QTimer::timeout, this, &MainWindow::updatePorts);
    _portTimer.start(PortUpdateDelay);

    _initBindings();
    _initConversionFlags();
    _setConnected(false);
    _setUploaded(false);

    _ui->image->setImageDimensions(QSize{EzGraver::ImageWidth, EzGraver::ImageHeight});
}

MainWindow::~MainWindow() {
    delete _ui;
}

void MainWindow::_initBindings() {
    connect(_ui->burnTime, &QSlider::valueChanged, [this](int const& v) { _ui->burnTimeLabel->setText(QString::number(v)); });

    connect(this, &MainWindow::connectedChanged, this, &MainWindow::enableControls);
    connect(this, &MainWindow::uploadedChanged,  this, &MainWindow::enableControls);

    connect(_ui->conversionFlags, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int index) {
        _ui->image->setConversionFlags(static_cast<Qt::ImageConversionFlags>(_ui->conversionFlags->itemData(index).toInt()));
    });

    connect(_ui->layered, &QCheckBox::toggled, _ui->selectedLayer, &QSpinBox::setEnabled);
    connect(_ui->layered, &QCheckBox::toggled, _ui->layerCount, &QSpinBox::setEnabled);
    connect(_ui->layered, &QCheckBox::toggled, _ui->image, &ImageLabel::setGrayscale);
    connect(_ui->layerCount, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), _ui->image, &ImageLabel::setLayerCount);
    connect(_ui->layerCount, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), _ui->selectedLayer, &QSpinBox::setMaximum);
    connect(_ui->selectedLayer, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), _ui->image, &ImageLabel::setLayer);

    auto uploadEnabled = [this] {
        _uploaded = false;
        _ui->upload->setEnabled(_ui->image->imageLoaded() && _connected && (!_ui->layered->isChecked() || _ui->selectedLayer->value() > 0));
        enableControls();
    };
    connect(this, &MainWindow::connectedChanged, uploadEnabled);
    connect(_ui->image, &ImageLabel::imageLoadedChanged, uploadEnabled);
    connect(_ui->selectedLayer, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), uploadEnabled);
    connect(_ui->layered, &QCheckBox::toggled, uploadEnabled);
    connect(_ui->keepAspectRatio, &QCheckBox::toggled, _ui->image, &ImageLabel::setKeepAspectRatio);
}

void MainWindow::enableControls()
{
    _ui->ports->setEnabled(!_connected);
    _ui->connect->setEnabled(!_connected);
    _ui->disconnect->setEnabled(_connected);

    _ui->home->setEnabled(_connected);
    _ui->up->setEnabled(_connected);
    _ui->left->setEnabled(_connected);
    _ui->center->setEnabled(_connected);
    _ui->right->setEnabled(_connected);
    _ui->down->setEnabled(_connected);
    _ui->preview->setEnabled(_connected);
    _ui->start->setEnabled(_connected && _uploaded);
    _ui->pause->setEnabled(_connected);
    _ui->reset->setEnabled(_connected);
}

void MainWindow::_initConversionFlags() {
    _ui->conversionFlags->addItem("DiffuseDither", Qt::DiffuseDither);
    _ui->conversionFlags->addItem("OrderedDither", Qt::OrderedDither);
    _ui->conversionFlags->addItem("ThresholdDither", Qt::ThresholdDither);
    _ui->conversionFlags->setCurrentIndex(0);
}

void MainWindow::_printVerbose(QString const& verbose) {
    _ui->verbose->appendPlainText(verbose);
}

void MainWindow::updatePorts() {
    QStringList ports{EzGraver::availablePorts()};
    ports.insert(0, "");

    QString original{_ui->ports->currentText()};
    _ui->ports->clear();
    _ui->ports->addItems(ports);

    if(ports.contains(original)) {
        _ui->ports->setCurrentText(original);
    }
}

void MainWindow::_loadImage(QString const& fileName) {
    _printVerbose(QString{"loading image: %1"}.arg(fileName));

    QImage image{};
    if(!image.load(fileName)) {
        _printVerbose("failed to load image");
        return;
    }

    _ui->image->setImage(image);
}

bool MainWindow::connected() const {
    return _connected;
}

bool MainWindow::uploaded() const {
    return _uploaded;
}

void MainWindow::_setConnected(bool connected) {
    _connected = connected;
    emit connectedChanged(connected);
}

void MainWindow::_setUploaded(bool uploaded) {
    _uploaded = uploaded;
    emit uploadedChanged(uploaded);
}

void MainWindow::bytesWritten(qint64 bytes) {
    _bytesWrittenProcessor(bytes);
}

void MainWindow::updateProgress(qint64 bytes) {
    qDebug() << "Bytes written:" << bytes;
    auto progress = _ui->progress->value() + bytes;
    _ui->progress->setValue(progress);
    if(progress >= _ui->progress->maximum()) {
        _printVerbose("upload completed");
        _bytesWrittenProcessor = [](qint64){};
    }
}

void MainWindow::on_connect_clicked() {
    try {
        _printVerbose(QString{"connecting to port %1"}.arg(_ui->ports->currentText()));
        _ezGraver = EzGraver::create(_ui->ports->currentText());
        _printVerbose("connection established successfully");
        _setConnected(true);
        _inData.clear();

        connect(_ezGraver->serialPort().get(), &QSerialPort::bytesWritten, this, &MainWindow::bytesWritten);
        connect(_ezGraver->serialPort().get(), &QSerialPort::readyRead, this, &MainWindow::readyRead);
    } catch(std::runtime_error const& e) {
        _printVerbose(QString{"Error: %1"}.arg(e.what()));
    }
}

void MainWindow::on_home_clicked() {
    _printVerbose("moving to home");
    _ezGraver->home();
}

void MainWindow::on_up_clicked() {
    _ezGraver->up();
}

void MainWindow::on_left_clicked() {
    _ezGraver->left();
}

void MainWindow::on_center_clicked() {
    _printVerbose("moving to center");
    _ezGraver->center();
}

void MainWindow::on_right_clicked() {
    _ezGraver->right();
}

void MainWindow::on_down_clicked() {
    _ezGraver->down();
}

void MainWindow::on_upload_clicked() {
    _printVerbose("erasing EEPROM");
    _ezGraver->erase();

    QImage image{_ui->image->pixmap()->toImage()};
    QTimer* eraseProgressTimer{new QTimer{this}};
    _ui->progress->setValue(0);
    _ui->progress->setMaximum(EzGraver::EraseTimeMs);
    _ui->image->resetBurnStatus();

    auto eraseProgress = std::bind(&MainWindow::_eraseProgressed, this, eraseProgressTimer, image);
    connect(eraseProgressTimer, &QTimer::timeout, eraseProgress);
    eraseProgressTimer->start(EraseProgressDelay);
}

void MainWindow::_eraseProgressed(QTimer* eraseProgressTimer, QImage const& image) {
    auto value = _ui->progress->value() + EraseProgressDelay;
    _ui->progress->setValue(value);
    if(value < EzGraver::EraseTimeMs) {
        return;
    }
    eraseProgressTimer->stop();

    _uploadImage(image);
}

void MainWindow::_uploadImage(QImage const& image) {
    _bytesWrittenProcessor = std::bind(&MainWindow::updateProgress, this, std::placeholders::_1);
    if (_ui->image->picW() <= 0 )
    {
        _printVerbose("cannot upload empty image");
        return;
    }
    _printVerbose("uploading image to EEPROM");
    auto bytes = _ezGraver->uploadImage(image);
    int maxProgress = _ui->image->burnCount();
    if (maxProgress == 0)
        maxProgress = EzGraver::ImageWidth * EzGraver::ImageHeight;
    _ui->progress->setMaximum(maxProgress);
    _ui->progress->setValue(bytes);
    _ui->image->resetBurnStatus();
    _ezGraver->requestReady();
}

void MainWindow::on_preview_clicked() {
    _printVerbose("drawing preview");
    _ezGraver->preview();
}

void MainWindow::on_start_clicked() {
    _printVerbose(QString{"starting engrave process with burn time %1"}.arg(_ui->burnTime->value()));
    _ui->progress->setValue(0);
    _ezGraver->start(_ui->burnTime->value());
}

void MainWindow::on_pause_clicked() {
    _printVerbose("pausing engrave process");
    _ezGraver->pause();
}

void MainWindow::on_reset_clicked() {
    _printVerbose("resetting engraver");
    _ui->image->resetBurnStatus();
    _ezGraver->reset();
}

void MainWindow::on_disconnect_clicked() {
    _printVerbose("disconnecting");
    _setConnected(false);
    _ezGraver.reset();
    _printVerbose("disconnected");
}

void MainWindow::on_image_clicked() {
    auto fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpeg *.jpg *.bmp)");
    if(!fileName.isNull()) {
        _loadImage(fileName);
        emit uploadedChanged(false);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if(event->mimeData()->hasUrls() && event->mimeData()->urls().count() == 1) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    QString fileName{event->mimeData()->urls()[0].toLocalFile()};
    _loadImage(fileName);
}

void MainWindow::readyRead()
{
    _inData += _ezGraver->serialPort()->read(1024);
    bool marked = false;
    for (int i = 0; _inData.size() > 0; ++i)
    {
        if ((_inData.size() >= 5) && (_inData[0] == '\xff'))
        {
            // Report about burned pixel
            int pic_x = (_inData[1]*100 + _inData[2]);
            int pic_y = (_inData[3]*100 + _inData[4]);
            _ui->progress->setValue( _ui->image->markBurnedPixel( pic_x, pic_y ) );
            marked = true;
            _inData.remove(0, 5);
        } else
        if ((_inData.size() > 0) && (_inData[0] == '\x66'))
        {
            // Report about complete burning
            _printVerbose("status - complete");
            _ui->progress->setValue(0);
            _ui->image->resetBurnStatus();
            _inData.remove(0, 1);
        } else
        if ((_inData.size() > 0) && (_inData[0] == '\x65'))
        {
            // Ready status
            _printVerbose("status - ready");
            _ui->progress->setValue(0);
            _setUploaded(true);
            _inData.remove(0, 1);
        } else
        {
            if (i == 0)
            {
                qDebug() << "received unknown data" << _inData.size() << " bytes: " << _inData.toHex();
                _inData.clear();
            }
            break;
        }
    }

    if (marked)
        _ui->image->updateInfoLayers();
}

