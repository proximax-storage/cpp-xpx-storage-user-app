#include "CustomLogsRedirector.h"

#include <QDebug>

CustomLogsRedirector::CustomLogsRedirector(std::ostream &stream)
:
    std::basic_streambuf<char>(),
    mStream(stream)
{
    mOldBuf = stream.rdbuf();
    stream.rdbuf(this);
}

CustomLogsRedirector::~CustomLogsRedirector()
{
    if (!mStr.empty())
    {
        qDebug() << mStr.c_str();
    }

    mStream.rdbuf(mOldBuf);
}

CustomLogsRedirector::int_type CustomLogsRedirector::overflow(int_type v)
{
    if (v == '\n')
    {
        qDebug() << mStr.c_str();
        mStr.erase(mStr.begin(), mStr.end());
    }
    else
    {
        mStr += v;
    }

    return v;
}

std::streamsize CustomLogsRedirector::xsputn(const char *p, std::streamsize n)
{
    mStr.append(p, p + n);
    long pos = 0;
    while (pos != static_cast<long>(std::string::npos))
    {
        pos = static_cast<long>(mStr.find('\n'));
        if (pos != static_cast<long>(std::string::npos))
        {
            std::string tmp(mStr.begin(), mStr.begin() + pos);
            qDebug() << mStr.c_str();
            mStr.erase(mStr.begin(), mStr.begin() + pos + 1);
        }
    }

    return n;
}