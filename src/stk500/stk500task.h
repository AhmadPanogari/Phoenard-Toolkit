#ifndef STK500TASK_H
#define STK500TASK_H

#include "stk500.h"
#include "sketchinfo.h"
#include "programdata.h"
#include <QStringList>
#include <QList>
#include "longfilenamegen.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <qmath.h>

#define SHOW_DOTNAMES 0

class stk500Task
{
public:
    stk500Task(QString title = "")
        : _hasError(false), _isCancelled(false), _progress(-1.0),
          _status(title + "..."), _title(title), _cancelSuppress(false),
          _isFinished(false), _usesFirmware(true) {}

    virtual ~stk500Task() {}
    virtual void run() = 0;
    virtual void init() {}
    void setProtocol(stk500 *protocol) { this->protocol = protocol; }
    bool hasError() { return _hasError; }
    bool isCancelled() { return (_isCancelled && !_cancelSuppress); }
    bool isFinished() { return _isFinished; }
    bool isSuccessful() { return !isCancelled() && !hasError(); }
    bool usesFirmware() { return _usesFirmware; }
    void setUsesFirmware(bool usesFirmware) { _usesFirmware = usesFirmware; }
    void suppressCancel(bool suppress) { _cancelSuppress = suppress; }
    bool isCancelSuppressed() { return _cancelSuppress; }
    const ProtocolException getError() { return _exception; }
    QString getErrorMessage() { return QString(_exception.what()); }
    void setError(ProtocolException exception);
    void cancel() { _isCancelled = true; }
    void finish() { _isFinished = true; }
    void setProgress(double progress) { _progress = progress; }
    double progress() { return _progress; }
    void setStatus(QString status);
    void showError();
    QString status();
    QString title() { return _title; }

    /* Helpful utility functions task implementations can use */
    DirectoryEntryPtr sd_findEntry(QString path, bool isDirectory, bool create);
    DirectoryEntryPtr sd_findEntry(DirectoryEntryPtr dirStartPtr, QString name, bool isDirectory, bool create);
    DirectoryEntryPtr sd_findDirstart(QString directoryPath);
    DirectoryEntryPtr sd_createEntry(DirectoryEntryPtr dirPtr, QString name, bool isDirectory, QStringList existingShortNames);
    QString sd_findShortName(QString name_base, QString name_ext, QStringList existingShortNames);

    QList<DirectoryInfo> sd_list(DirectoryEntryPtr startPtr);
    void sd_allocEntries(DirectoryEntryPtr startPos, int oldLength, int newLength);
    bool sd_remove(QString fileName, bool fileIsDir);
protected:
    stk500 *protocol;

private:
    ProtocolException _exception;
    bool _hasError;
    bool _isCancelled;
    double _progress;
    QString _status;
    QString _title;
    bool _cancelSuppress;
    bool _isFinished;
    bool _usesFirmware;
    QMutex _sync;
};

class stk500ListSubDirs : public stk500Task {
public:
    stk500ListSubDirs(QString directoryPath) : stk500Task("Listing files"), directoryPath(directoryPath) {}
    virtual void run();

    QString directoryPath;
    QList<DirectoryInfo> result;
};

class stk500SaveFiles : public stk500Task {
public:
    stk500SaveFiles(QString sourceFile, QString destFile) : stk500Task("Reading from device"), sourceFile(sourceFile), destFile(destFile) {}
    virtual void run();
    void saveFile(DirectoryEntry fileEntry, QString sourceFilePath, QString destFilePath, double progStart, double progTotal);
    void saveFolder(DirectoryEntryPtr dirStartPtr, QString sourceFilePath, QString destFilePath, double progStart, double progTotal);

    QString sourceFile;
    QString destFile;
};

class stk500ImportFiles : public stk500Task {
public:
    stk500ImportFiles(QString sourceFile, QString destFile) : stk500Task("Writing to device"), sourceFile(sourceFile), destFile(destFile) {}
    virtual void run();
    void importFile(DirectoryEntryPtr dirStartPtr, QString sourceFilePath, QString destFilePath, double progStart, double progTotal);
    void importFolder(DirectoryEntryPtr dirStartPtr, QString sourceFilePath, QString destFilePath, double progStart, double progTotal);

    QString sourceFile;
    QString destFile;
    DirectoryEntry destEntry;
};

/*
 * Deletes all file names specified in the fileNames list
 * It then returns a new list of directories remaining in the directory
 */
class stk500Delete : public stk500Task {
public:
    stk500Delete(QString directoryPath, QStringList fileNames)
        : stk500Task("Deleting file"), directoryPath(directoryPath), fileNames(fileNames) {}
    virtual void run();
    void deleteInFolder(DirectoryEntryPtr dirStartPtr, QStringList fileNames, bool all, bool storeFiles);

    QString directoryPath;
    QStringList fileNames;
    QList<DirectoryInfo> currentFiles;
};

class stk500Rename : public stk500Task {
public:
    stk500Rename(QString filePath, QString newFileName, bool ignoreMissing = false)
        : stk500Task("Renaming file"), filePath(filePath), newFileName(newFileName), ignoreMissing(ignoreMissing), isMissing(false) {}
    virtual void run();

    QString filePath;
    QString newFileName;
    QList<DirectoryInfo> currentFiles;
    bool ignoreMissing;
    bool isMissing;
};

class stk500RenameVolume : public stk500Task {
public:
    stk500RenameVolume(QString volumeName) : stk500Task("Renaming volume"), volumeName(volumeName) {}
    virtual void run();

    QString volumeName;
};

class stk500ListSketches : public stk500Task {
public:
    stk500ListSketches() : stk500Task("Listing sketches") {}
    virtual void run();

    QList<SketchInfo> sketches;
    QString currentSketch;
};

class stk500LaunchSketch : public stk500Task {
public:
    stk500LaunchSketch(QString sketchName) : stk500Task("Launching sketch"), sketchName(sketchName) {}
    virtual void run();

    QString sketchName;
};

class stk500LoadIcon : public stk500Task {
public:
    stk500LoadIcon(SketchInfo &sketch) : stk500Task("Loading icon"), sketch(sketch) {}
    virtual void run();

    SketchInfo sketch;
};

class stk500UpdateRegisters : public stk500Task {
public:
    stk500UpdateRegisters(ChipRegisters &reg) : stk500Task("Updating registers"), reg(reg), read(true), readADC(true) {}
    virtual void run();

    ChipRegisters reg;
    bool read;
    bool readADC;
};

class stk500Upload : public stk500Task {
public:
    stk500Upload(const ProgramData &data)
        : stk500Task("Uploading"), data(data) {}
    virtual void run();
    virtual void init();

    ProgramData data;
};

class stk500BeginSerial : public stk500Task {
public:
    stk500BeginSerial(int idxFrom, int idxTo, quint32 baudRate) :
        stk500Task("Starting serial"), idxFrom(idxFrom), idxTo(idxTo), baudRate(baudRate) {}
    virtual void run();

    int idxFrom;
    int idxTo;
    quint32 baudRate;
};

#endif // STK500TASK_H
