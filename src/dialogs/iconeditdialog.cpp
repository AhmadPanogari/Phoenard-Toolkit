#include "iconeditdialog.h"
#include "ui_iconeditdialog.h"
#include "../stk500/sketchinfo.h"
#include <QDebug>

IconEditDialog::IconEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IconEditDialog)
{
    ui->setupUi(this);

    Qt::WindowFlags flags = this->windowFlags();
    flags |= Qt::WindowMaximizeButtonHint;
    flags &= ~Qt::WindowContextHelpButtonHint;
    setWindowFlags( flags );
}

PHNImage &IconEditDialog::image() {
    return ui->image->image();
}

IconEditDialog::~IconEditDialog()
{
    delete ui;
}

void IconEditDialog::loadIcon(const char* iconData) {
    QByteArray data(iconData, 512);
    image().loadData(64, 64, 1, data);
}

void IconEditDialog::saveIcon(char* iconData) {
    QByteArray data = image().saveData();
    memcpy(iconData, data.data(), 512);
}

void IconEditDialog::setWindowTitle(const QString &title) {
    this->title = title;
    updateTitle();
}

void IconEditDialog::closeEvent(QCloseEvent *event) {
    if (image().isModified()) {
        // Ask the user if we really want to save this
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Save icon", "The icon was changed. Do you want to save?",
                                        QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

        if (reply == QMessageBox::Cancel) {
            event->ignore();
        } else if (reply == QMessageBox::Yes) {
            this->accept();
        } else {
            this->reject();
        }
    } else {
        this->reject();
    }
}

void IconEditDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    image().resetModified();
    updateTitle();
}

void IconEditDialog::on_image_mouseChanged(QPoint point, Qt::MouseButtons buttons) {
    if (buttons & Qt::LeftButton) {
        image().setPixel(point.x(), point.y(), QColor(Qt::black));
    }
    if (buttons & Qt::RightButton) {
        image().setPixel(point.x(), point.y(), QColor(Qt::white));
    }
    QPainterPath sel;
    sel.addRect(point.x(), point.y(), 1, 1);
    ui->image->setSelection(sel);
}

void IconEditDialog::on_image_imageChanged() {
    ui->preview->setPixmap(image().pixmap());
    updateTitle();
}

void IconEditDialog::updateTitle() {
    if (image().isModified()) {
        QDialog::setWindowTitle(this->title + "*");
    } else {
        QDialog::setWindowTitle(this->title);
    }
}

void IconEditDialog::on_cancelBtn_clicked()
{
    this->reject();
}

void IconEditDialog::on_acceptBtn_clicked()
{
    this->accept();
}

void IconEditDialog::on_importBtn_clicked()
{
    /* Browse a file on the local filesystem */
    QFileDialog dialog(this);
    dialog.setWindowTitle("Select the image file to load");
    if (!dialog.exec()) {
        return;
    }
    QString filePath = dialog.selectedFiles().at(0);
    QFile file(filePath);
    QByteArray data;
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
        file.close();
    } else {
        //TODO: MESSAGE
        return;
    }

    // Load the data
    PHNImage &image = this->image();
    image.loadData(data);
    if (image.isNull()) {
        while (data.length() < 512) {
            data.append((char) 0x00);
        }
        image.loadData(64, 64, 1, data);
    }
    image.resize(64, 64);
    image.setFormat(LCD1);
    image.setColors(QList<QColor>() << QColor(Qt::black) << QColor(Qt::white));
    image.setHeader(false);
}

void IconEditDialog::on_resetBtn_clicked()
{
    // Load the default icon image
    QByteArray iconData((const char*) SKETCH_DEFAULT_ICON, sizeof(SKETCH_DEFAULT_ICON));
    PHNImage &image = this->image();
    image.loadData(64, 64, 1, iconData);
    image.setHeader(false);
}
