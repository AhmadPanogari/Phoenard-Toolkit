#include "serialmonitorwidget.h"
#include "ui_serialmonitorwidget.h"
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QClipboard>

serialmonitorwidget::serialmonitorwidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::serialmonitorwidget)
{
    ui->setupUi(this);

    this->updateTimer = new QTimer(this);
    connect(this->updateTimer, SIGNAL(timeout()), this, SLOT(readSerialOutput()));
    this->updateTimer->start(20);

    this->mode = STK500::SKETCH;
    this->screen.cmd_len = 0;
    this->screenEnabled = false;
    ui->outputImage->setVisible(false);
    resetScreen();

    // Setup fonts
    QFont serialFont("Inconsolata", 10);
    ui->outputText->setFont(serialFont);
    ui->messageTxt->setFont(serialFont);

    // Attach right-click menu to the output log text edit
    ui->outputText->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->outputText, SIGNAL(customContextMenuRequested(QPoint)),
        this, SLOT(showOutputContextMenu(const QPoint&)));

    // Attach right-click menu to the output image viewer
    ui->outputImage->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->outputImage, SIGNAL(customContextMenuRequested(const QPoint&)),
        this, SLOT(showImageContextMenu(const QPoint&)));
}

serialmonitorwidget::~serialmonitorwidget()
{
    delete ui;
}

void serialmonitorwidget::setSerial(stk500Serial *serial)
{
    this->serial = serial;

    connect(serial, SIGNAL(serialOpened()),
            this,    SLOT(clearOutputText()),
            Qt::QueuedConnection);
}

void serialmonitorwidget::setScreenShare(bool enabled)
{
    if (this->screenEnabled != enabled) {
        this->screenEnabled = enabled;
        this->openSerial();
    }
    ui->outputImage->setVisible(enabled);
    ui->outputText->setVisible(!enabled);
}


void serialmonitorwidget::setMode(STK500::State mode)
{
    if (this->mode != mode) {
        this->mode = mode;
        this->openSerial();
    }
}

void serialmonitorwidget::openSerial()
{
    if (!ui->runSketchCheck->isChecked()) {
        serial->closeSerial();
    } else if (this->screenEnabled) {
        serial->openSerial(115200, this->mode);
    } else {
        QString baudSel = ui->baudrateBox->currentText();
        QString baud_pfix = " baud";
        if (baudSel.endsWith(baud_pfix)) {
            baudSel.chop(baud_pfix.length());
        }
        serial->openSerial(baudSel.toInt(), this->mode);
    }
}

void serialmonitorwidget::showImageContextMenu(const QPoint& pos) {
    // Map the point to a point on the screen
    QPoint globalPos = ui->outputImage->mapToGlobal(pos);

    // Show a right-click menu with options for the image
    QMenu menu;
    QAction* copyClip = menu.addAction("Copy to clipboard");
    QAction* selected = menu.exec(globalPos);

    // Execute selected actions
    if (selected == copyClip) {
        QClipboard *clipboard = QApplication::clipboard();
        QPixmap pixmap = ui->outputImage->image().pixmap();
        clipboard->setPixmap(pixmap);
    }
}

void serialmonitorwidget::showOutputContextMenu(const QPoint& pos) {
    // Map the point to a point on the screen
    QPoint globalPos = ui->outputText->mapToGlobal(pos);

    // Create a standard context menu, and append additional actions
    QMenu* menu = ui->outputText->createStandardContextMenu();
    menu->addSeparator();
    QAction* clearAct = menu->addAction("Clear");
    QAction* selected = menu->exec(globalPos);

    // Execute selected actions
    if (selected == clearAct) {
        this->clearOutputText();
    }
}

/* Re-open Serial in the right mode when running is toggled */
void serialmonitorwidget::on_runSketchCheck_toggled(bool)
{
    openSerial();
}

/* Re-open Serial in the right baud rate when rate is changed */
void serialmonitorwidget::on_baudrateBox_activated(int)
{
    openSerial();
}

/* Redirect enter presses to the send button */
void serialmonitorwidget::on_messageTxt_returnPressed()
{
    ui->sendButton->click();
}

/* Send message to Serial when send button is clicked */
void serialmonitorwidget::on_sendButton_clicked()
{
    QString message = ui->messageTxt->text();
    ui->messageTxt->clear();
    if (message.isEmpty()) {
        return;
    }

    // Append newlines
    int newline_idx = ui->lineEndingBox->currentIndex();
    if (newline_idx == 2 || newline_idx == 3) {
        message.append('\r'); // Carriage return
    }
    if (newline_idx == 1 || newline_idx == 3) {
        message.append('\n'); // Newline
    }

    // Send it to Serial
    serial->write(message);
}

/* Clear output text - when serial opens */
void serialmonitorwidget::clearOutputText() {
    ui->outputText->clear();
    this->recData.clear();
    resetScreen();
}

/* Save all data in the receive buffer to a file */
bool serialmonitorwidget::saveReceivedData(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(this->recData);
    file.close();
    return true;
}

/* Writes raw data to the device */
void serialmonitorwidget::sendData(const QByteArray &data) {
    serial->write(data.data(), data.length());
}

/* Reads data contained in a file and writes it to the device */
bool serialmonitorwidget::sendFileData(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    sendData(data);
    return true;
}

/* Read Serial output and display it in the text area */
void serialmonitorwidget::readSerialOutput()
{
    char buff[1024];
    int len = serial->read(buff, sizeof(buff));
    if (len) {

        // Append data received to the output buffer, in case we want to save
        this->recData.append(buff, len);

        // Handle the incoming data
        if (this->screenEnabled) {
            for (int i = 0; i < len; i++) {
                receiveScreen(buff[i]);
            }
        } else {
            // First filter out invalid characters from the string
            for (int i = 0; i < len; i++) {
                if (buff[i] == 0) buff[i] = ' ';
            }

            QString myString = QString::fromLatin1(buff, len);
            myString = myString.remove('\r');

            if (ui->autoScrollCheck->isChecked()) {

                ui->outputText->moveCursor (QTextCursor::End);
                ui->outputText->insertPlainText(myString);
                ui->outputText->moveCursor (QTextCursor::End);
            } else {
                int old_scroll = ui->outputText->verticalScrollBar()->value();
                QTextCursor old_cursor = ui->outputText->textCursor();
                ui->outputText->moveCursor (QTextCursor::End);
                ui->outputText->insertPlainText(myString);
                ui->outputText->setTextCursor(old_cursor);
                ui->outputText->verticalScrollBar()->setValue(old_scroll);
            }
        }
    }
}

/* Resets the screen to all black */
void serialmonitorwidget::resetScreen() {
    ui->outputImage->image().create(320, 240);
}

/* Receives a single byte as part of the screen serial protocol */
void serialmonitorwidget::receiveScreen(quint8 byte)
{
    if (this->screen.cmd_len) {
        // Refresh buffer
        this->screen.cmd_buff[this->screen.cmd_buff_index++] = byte;
        if (this->screen.cmd_buff_index == this->screen.cmd_len) {
            quint16 data = *((quint16*) this->screen.cmd_buff);
            quint32 data_cnt = 1;
            if (this->screen.cmd_len == 6) {
                data_cnt = *((quint32*) (this->screen.cmd_buff + 2));

                // Make sure to limit it to the amount of pixels we have
                // In case it glitches, we won't be stuck in a loop
                if (data_cnt > (320*240)) data_cnt = (320*240);
            }
            this->screen.cmd_len = 0;
            // Process the command with the data known
            switch (this->screen.cmd) {
            case 0x22:
                // CGRAM read/write
                for (quint32 i = 0; i < data_cnt; i++) {
                    receivePixel(data);
                }
                break;
            case 0x03:
                // Read entrymode
                this->screen.entrymode = data;
                break;
            case 0x50:
                // Horizontal start viewport
                this->screen.view_vb = 239 - data;
                break;
            case 0x51:
                // Horizontal end viewport
                this->screen.view_va = 239 - data;
                break;
            case 0x52:
                // Vertical start viewport
                this->screen.view_hb = 319 - data;
                break;
            case 0x53:
                // Vertical end viewport
                this->screen.view_ha = 319 - data;
                break;
            case 0x20:
                // Horizontal cursor (y)
                this->screen.cur_y = 239 - data;
                break;
            case 0x21:
                // Vertical cursor (z)
                this->screen.cur_x = 319 - data;
                break;

            default:
                // Do nothing
                qDebug() << "SCREEN ERROR " << screen.cmd;
                break;
            }
        }
    } else {
        this->screen.cmd_buff_index = 0;
        this->screen.cmd_len = 0;
        if (byte == 0x00) {
            // Screen reset
            this->screen.view_ha = 0;
            this->screen.view_hb = 319;
            this->screen.view_va = 0;
            this->screen.view_vb = 239;
            this->resetScreen();
        } else if (byte == 0xFF) {
            this->screen.cmd_len = 2;
        } else if (byte == 0xFE) {
            this->screen.cmd_len = 6;
        } else {
            this->screen.cmd = byte;
        }
    }
}

/* Receives a single pixel */
void serialmonitorwidget::receivePixel(quint16 color)
{
    // Write pixel to the screen image
    this->ui->outputImage->image().setPixel(screen.cur_x, screen.cur_y, color565_to_rgb(color));

    // Next pixel - use entrymode and view port
    switch (screen.entrymode) {
    case 0x1008:
        if (moveCursor_x(1)) moveCursor_y(1);
        break;
    case 0x1020:
        if (moveCursor_y(1)) moveCursor_x(-1);
        break;
    case 0x1038:
        if (moveCursor_x(-1)) moveCursor_y(1);
        break;
    case 0x1010:
        if (moveCursor_y(-1)) moveCursor_x(1);
        break;
    case 0x1018:
        if (moveCursor_x(1)) moveCursor_y(-1);
        break;
    case 0x1000:
        if (moveCursor_y(1)) moveCursor_x(1);
        break;
    case 0x1028:
        if (moveCursor_x(-1)) moveCursor_y(-1);
        break;
    case 0x1030:
        if (moveCursor_y(-1)) moveCursor_x(-1);
        break;
    }
}

bool serialmonitorwidget::moveCursor_x(qint8 dx) {
    screen.cur_x += dx;
    if (screen.cur_x > screen.view_hb) {
        screen.cur_x = screen.view_ha;
        return true;
    } else if (screen.cur_x < screen.view_ha) {
        screen.cur_x = screen.view_hb;
        return true;
    } else {
        return false;
    }
}

bool serialmonitorwidget::moveCursor_y(qint8 dy) {
    screen.cur_y += dy;
    if (screen.cur_y > screen.view_vb) {
        screen.cur_y = screen.view_va;
        return true;
    } else if (screen.cur_y < screen.view_va) {
        screen.cur_y = screen.view_vb;
        return true;
    } else {
        return false;
    }
}
