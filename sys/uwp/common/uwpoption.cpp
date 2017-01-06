#include "uwpoption.h"

#include <fstream>
#include <iostream>

namespace Nethack {

    void Options::Load(std::string & filePath)
    {
        m_filePath = filePath;

        std::string optionsEqual = "OPTIONS=";

        m_options.clear();

        std::fstream input(m_filePath.c_str(), std::fstream::in);
        if (input.is_open())
        {
            std::string line;

            while (std::getline(input, line))
            {
                if (line.compare(0, optionsEqual.size(), optionsEqual.c_str()) != 0)
                    continue;

                std::size_t colonPosition = line.find(':', optionsEqual.size());

                std::string name = line.substr(optionsEqual.size(), colonPosition - optionsEqual.size());

                if (name.length() == 0)
                    continue;

                std::string value;

                if (colonPosition != std::string::npos)
                {
                    value = line.substr(colonPosition + 1, std::string::npos);
                }

                m_options.push_back(Option(name, value));
            }

            input.close();
        }

    }

    void Options::Store()
    {
        std::fstream output(m_filePath.c_str(), std::fstream::out | std::fstream::trunc);

        if (output.is_open())
        {
            for (auto & o : m_options)
            {
                output << "OPTIONS=";
                output << o.m_name;
                if (o.m_value.length() > 0)
                {
                    output << ":";
                    output << o.m_value;
                }
                output << "\n";
            }

            output.close();
        }
    }

    std::string Options::GetString(void)
    {
        std::string options;

        for (auto & o : m_options)
        {
            if (options.length() > 0)
                options += ",";

            options += o.m_name;
            if (o.m_value.length() > 0)
            {
                options += ":";
                options += o.m_value;
            }
        }

        return options;
    }

    void Options::Remove(std::string & name)
    {
        auto i = m_options.begin();
        while (i != m_options.end())
        {
            auto & o = *i;
            if (name.compare(o.m_name) == 0)
            {
                i = m_options.erase(i);
            }
            else
                i++;
        }
    }


}