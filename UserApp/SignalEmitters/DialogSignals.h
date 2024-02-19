#ifndef DIALOGSIGNALS_H
#define DIALOGSIGNALS_H

#include <QObject>

class DialogSignals : public QObject
{
public:
    Q_OBJECT

public:
    explicit DialogSignals(QObject* parent);
    ~DialogSignals() override;

public:
signals:
    void addDownloadChannel(const std::string&             channelName,
                            const std::string&             channelKey,
                            const std::string&             driveKey,
                            const std::vector<std::string> allowedPublicKeys);

};


#endif // DIALOGSIGNALS_H
