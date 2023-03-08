#pragma once

#include <windows.h>

namespace base {

    class Event
    {
    public:
        explicit Event(BOOL bInitiallyOwn = FALSE, BOOL bManualReset = FALSE,
            LPCTSTR lpszNAme = NULL, LPSECURITY_ATTRIBUTES lpsaAttribute = NULL);

    public:
        BOOL SetEvent();
        BOOL PulseEvent();
        BOOL ResetEvent();

    public:
        virtual ~Event();
        operator HANDLE() const { return m_hObject; }
        HANDLE  m_hObject;
    };

}
