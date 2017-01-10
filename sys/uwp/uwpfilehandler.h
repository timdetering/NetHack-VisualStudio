#pragma once

#include <windows.h>
#include <string>
#include "common\uwplock.h"
#include "common\uwpconditionvariable.h"

namespace Nethack
{

    ref class FileHandler sealed
    {
    public:

        static property FileHandler ^ s_instance;

        FileHandler();

        void SetCoreDispatcher(Windows::UI::Core::CoreDispatcher ^ coreDispatcher)
        {
            m_coreDispatcher = coreDispatcher;
        }

        void SaveFilePicker(Platform::String ^ fileText, Platform::String ^ fileName, Platform::String ^ extension);
        Platform::String ^ LoadFilePicker(Platform::String ^ extension);

    private:

        Windows::UI::Core::CoreDispatcher ^ m_coreDispatcher;
        Nethack::Lock m_lock;
        Nethack::ConditionVariable m_conditionVariable;
        Windows::Storage::StorageFile ^ m_file;

    };

}

