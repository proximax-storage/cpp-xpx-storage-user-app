#pragma once

#include <map>
#include <QString>

class ErrorCodeTranslator
{
    std::map<QString, QString> m_dictionary;

public:
    ErrorCodeTranslator();

    const QString translate(const QString& errorCode) const
    {
        auto it = m_dictionary.find(errorCode);

        if(it == m_dictionary.end())
        {
            return errorCode;
        }

        return it->second;
    }
};

inline ErrorCodeTranslator gErrorCodeTranslator;
