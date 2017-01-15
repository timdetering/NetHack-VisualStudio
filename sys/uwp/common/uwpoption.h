#pragma once

#include <string>
#include <vector>

namespace Nethack
{
    class Options
    {
    public:

        void Load(std::string & filePath);
        void Store();
        std::string GetString();
    
        std::vector<std::string> m_options;
        std::string m_filePath;
    };
}
