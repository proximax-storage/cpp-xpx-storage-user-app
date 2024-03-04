#pragma once

#include <map>
#include <string>

class ErrorCodeTranslator
{
    std::map<std::string, std::string> m_dictionary;

public:
    ErrorCodeTranslator();

    const std::string translate(const std::string& errorCode) const
    {
        auto it = m_dictionary.find(errorCode);

        if(it == m_dictionary.end())
        {
            return errorCode.c_str();
        }

        return it->second;
    }
};

inline ErrorCodeTranslator gErrorCodeTranslator;
