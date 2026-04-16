#include "trainmanager.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

TrainManager::TrainManager(QObject *parent)
    : QObject(parent)
{
    connect(&process, &QProcess::readyReadStandardOutput, this, &TrainManager::onStdOut);
    connect(&process, &QProcess::readyReadStandardError, this, &TrainManager::onStdErr);
    connect(&process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &TrainManager::onFinished);
}

bool TrainManager::start(const QString &workingDir, const QString &datasetYamlPath)
{
    if(process.state() != QProcess::NotRunning)
    {
        emit logLine("Training is already running.");
        return false;
    }

    const QFileInfo scriptInfo(QDir(workingDir).absoluteFilePath("scripts/train_yolo.py"));
    if(!scriptInfo.exists())
    {
        emit trainFailedToStart("scripts/train_yolo.py was not found.");
        return false;
    }

    QString program;
    QStringList args;

#ifdef Q_OS_WIN
    // Prefer Python launcher on Windows to avoid Microsoft Store python alias issues.
    const QString pyPath = QStandardPaths::findExecutable("py");
    if(!pyPath.isEmpty())
    {
        program = pyPath;
        args << "-3";
    } else
    {
        const QString pythonPath = QStandardPaths::findExecutable("python");
        if(pythonPath.isEmpty())
        {
            emit trainFailedToStart(
                "Python launcher 'py' and 'python' were not found in PATH. "
                "Install Python from python.org and ensure PATH is configured.");
            return false;
        }
        program = pythonPath;
    }
#else
    const QString python3Path = QStandardPaths::findExecutable("python3");
    const QString pythonPath = QStandardPaths::findExecutable("python");
    if(!python3Path.isEmpty())
    {
        program = python3Path;
    }
    else if(!pythonPath.isEmpty())
    {
        program = pythonPath;
    }
    else
    {
        emit trainFailedToStart("Python was not found in PATH.");
        return false;
    }
#endif

    args << scriptInfo.absoluteFilePath() << "--data" << datasetYamlPath;

    process.setProgram(program);
    process.setArguments(args);
    process.setWorkingDirectory(workingDir);
    process.start();

    if(!process.waitForStarted(2000))
    {
        emit trainFailedToStart(process.errorString());
        return false;
    }

    emit logLine("Training process has started.");
    return true;
}

bool TrainManager::isRunning(void) const
{
    return process.state() != QProcess::NotRunning;
}

void TrainManager::onStdOut(void)
{
    const QString text = QString::fromLocal8Bit(process.readAllStandardOutput());
    if(!text.isEmpty())
    {
        emit logLine(text.trimmed());
    }
}

void TrainManager::onStdErr(void)
{
    const QString text = QString::fromLocal8Bit(process.readAllStandardError());
    if(!text.isEmpty())
    {
        emit logLine(text.trimmed());
    }
}

void TrainManager::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    emit trainFinished(exitCode, exitStatus);
}
