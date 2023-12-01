#include "CustomLogsRedirector.h"

#include <QDebug>
#include <QRegularExpression>

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
        qDebug() << QString::fromStdString(mStr);
    }

    mStream.rdbuf(mOldBuf);
}

CustomLogsRedirector::int_type CustomLogsRedirector::overflow(int_type v)
{
    if (v == '\n')
    {
        qDebug() << QString::fromStdString(mStr);
        mStr.clear();
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
            qDebug() << QString::fromStdString(mStr);
            mStr.erase(mStr.begin(), mStr.begin() + pos + 1);
        }
    }

    return n;
}