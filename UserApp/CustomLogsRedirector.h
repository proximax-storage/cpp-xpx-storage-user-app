#ifndef CUSTOMSTRINGSTREAM_H
#define CUSTOMSTRINGSTREAM_H

#include <sstream>

class CustomLogsRedirector : public std::basic_streambuf<char>
{
public:
    explicit CustomLogsRedirector(std::ostream &stream);
    virtual ~CustomLogsRedirector();

protected:
    virtual int_type overflow(int_type v);
    virtual std::streamsize xsputn(const char *p, std::streamsize n);

private:
    std::ostream &mStream;
    std::streambuf *mOldBuf;
    std::string mStr;
};

#endif //CUSTOMSTRINGSTREAM_H
