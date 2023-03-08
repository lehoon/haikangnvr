#include "Event.h"

namespace base {

	Event::Event(BOOL bInitiallyOwn, BOOL bManualReset, LPCTSTR pstrName,
		LPSECURITY_ATTRIBUTES lpsaAttribute)
	{
		m_hObject = ::CreateEvent(lpsaAttribute, bManualReset,
			bInitiallyOwn, pstrName);
		if (m_hObject == NULL) {
			//AfxThrowResourceException();
		}
	}

	Event::~Event()
	{
		if (NULL != m_hObject) {
			::CloseHandle(m_hObject);
			m_hObject = NULL;
		}
	}

	BOOL Event::SetEvent()
	{
		return ::SetEvent(m_hObject);
	}
	BOOL Event::PulseEvent()
	{
		return ::PulseEvent(m_hObject);
	}
	BOOL Event::ResetEvent()
	{
		return ::ResetEvent(m_hObject);
	}
}
