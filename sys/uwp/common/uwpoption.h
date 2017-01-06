#pragma once

#include <string>
#include <vector>

namespace Nethack
{
    class Option
    {
    public:
        Option(const std::string & name, const std::string & value) : m_name(name), m_value(value) {}

        std::string m_name;
        std::string m_value;
        
    };

    class Options
    {
    public:

        void Load(std::string & filePath);
        void Store();
        std::string GetString();
        void Remove(std::string & name);
    
        std::vector<Option> m_options;
        std::string m_filePath;
    };
}
