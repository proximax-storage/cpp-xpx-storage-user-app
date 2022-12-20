#ifndef DOWNLOADINFO_H
#define DOWNLOADINFO_H

#include <string>
#include <array>
#include "drive/ActionList.h"

class DownloadInfo {
    public:
        DownloadInfo();
        ~DownloadInfo();

    public:
        std::array<uint8_t,32> getHash() const;
        void setHash(const std::array<uint8_t,32>& hash);

        std::string getDownloadChannelKey() const;
        void setDownloadChannelKey(const std::string& key);

        std::string getFileName() const;
        void setFileName(const std::string& name);

        std::string getSaveFolder() const;
        void setSaveFolder(const std::string& folder);

        int getProgress() const;
        void setProgress(int progress);

        sirius::drive::lt_handle getHandle() const;
        void setHandle(const sirius::drive::lt_handle& handle);

        bool isCompleted() const;
        void setCompleted(bool state);

    public:
        template<class Archive>
        void serialize( Archive &ar )
        {
            ar( m_hash, m_channelKey, m_fileName, m_saveFolder, m_isCompleted );
        }

    private:
        std::array<uint8_t,32>   m_hash;
        std::string              m_channelKey;
        std::string              m_fileName;
        std::string              m_saveFolder;
        bool                     m_isCompleted = false;
        int                      m_progress = 0; // m_progress==1001 means completed
        sirius::drive::lt_handle m_ltHandle;
};


#endif //DOWNLOADINFO_H
