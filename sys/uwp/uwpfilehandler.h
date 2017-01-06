#pragma once

#include <windows.h>
#include <string>
#include "common\uwplock.h"
#include "common\uwpconditionvariable.h"

namespace Nethack
{

    public ref class FileHandler sealed
    {
    public:

        static property FileHandler ^ s_instance;

        FileHandler();

        void SetCoreDispatcher(Windows::UI::Core::CoreDispatcher ^ coreDispatcher)
        {
            m_coreDispatcher = coreDispatcher;
        }

        void SaveFilePicker(Platform::String ^ fileText);
        Platform::String ^ LoadFilePicker(void);

    private:

        Windows::UI::Core::CoreDispatcher ^ m_coreDispatcher;
        Nethack::Lock m_lock;
        Nethack::ConditionVariable m_conditionVariable;
        Windows::Storage::StorageFile ^ m_file;

    };

}

