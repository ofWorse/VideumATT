#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class TrainManager : public QObject
{
    Q_OBJECT

private:
    QProcess process;

public:
    explicit TrainManager(QObject *parent = nullptr);
    bool start(const QString &workingDir, const QString &datasetYamlPath);
    bool isRunning(void) const;

signals:
    void logLine(const QString &line);
    void trainFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void trainFailedToStart(const QString &errorText);

private slots:
    void onStdOut(void);
    void onStdErr(void);
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
};

