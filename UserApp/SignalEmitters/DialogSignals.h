#ifndef DIALOGSIGNALS_H
#define DIALOGSIGNALS_H

#include <QObject>

class DialogSignals : public QObject
{
public:
    Q_OBJECT

public:
    explicit DialogSignals(QObject* parent) : QObject(parent) {}
    ~DialogSignals() override {}

public:
signals:
    void addDownloadChannel(const QString&             channelName,
                            const QString&             channelKey,
                            const QString&             driveKey,
                            const std::vector<QString> allowedPublicKeys,
                            bool                       isForEasyDownload);

};


#endif // DIALOGSIGNALS_H
