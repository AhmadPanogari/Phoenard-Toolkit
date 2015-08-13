#-------------------------------------------------
#
# Project created by QtCreator 2014-11-10T10:37:13
#
#-------------------------------------------------

QT       += core gui
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Phoenard_Toolkit
TEMPLATE = app

win32:RC_ICONS += App_Icon.ico

SOURCES += main.cpp\
        mainwindow.cpp \
    stk500/stk500.cpp \
    stk500/stk500task.cpp \
    stk500/stk500serial.cpp \
    stk500/longfilenamegen.cpp \
    imaging/quantize.cpp \
    colorselect.cpp \
    controls/menubutton.cpp \
    controls/imageviewer.cpp \
    controls/sdbrowserwidget.cpp \
    controls/serialmonitorwidget.cpp \
    controls/sketchlistwidget.cpp \
    dialogs/progressdialog.cpp \
    dialogs/qmultifiledialog.cpp \
    dialogs/formatselectdialog.cpp \
    dialogs/codeselectdialog.cpp \
    dialogs/iconeditdialog.cpp \
    stk500/stk500settings.cpp \
    imaging/phnimage.cpp \
    imaging/imageformat.cpp \
    dialogs/asknamedialog.cpp \
    stk500/sketchinfo.cpp

HEADERS  += mainwindow.h \
    stk500/stk500.h \
    stk500/stk500_fat.h \
    stk500/stk500task.h \
    stk500/stk500serial.h \
    stk500/longfilenamegen.h \
    stk500/stk500command.h \
    imaging/quantize.h \
    colorselect.h \
    controls/menubutton.h \
    controls/imageviewer.h \
    controls/sdbrowserwidget.h \
    controls/serialmonitorwidget.h \
    controls/sketchlistwidget.h \
    dialogs/progressdialog.h \
    dialogs/qmultifiledialog.h \
    dialogs/formatselectdialog.h \
    dialogs/codeselectdialog.h \
    dialogs/iconeditdialog.h \
    stk500/sketchinfo.h \
    mainmenutab.h \
    stk500/stk500settings.h \
    imaging/phnimage.h \
    imaging/imageformat.h \
    dialogs/asknamedialog.h

FORMS    += mainwindow.ui \
    progressdialog.ui \
    dialogs/formatselectdialog.ui \
    dialogs/codeselectdialog.ui \
    dialogs/iconeditdialog.ui \
    controls/serialmonitorwidget.ui \
    controls/sketchlistwidget.ui \
    dialogs/asknamedialog.ui

OTHER_FILES +=

RESOURCES += \
    icons/icons.qrc
