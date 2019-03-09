#pragma once

#ifndef _AVP_SYNC_H_
#define _AVP_SYNC_H_

#include "pch.h"

class AVPMutex
{
public:
	AVPMutex() : m_bInitialized(false) { ZeroMemory(&m_cs, sizeof(m_cs)); }
	~AVPMutex() { Destroy(); }
	BOOL Create()
	{
		BOOL ret = InitializeCriticalSectionAndSpinCount(&m_cs, 0x400);
		m_bInitialized = ret ? true : false;
		return ret;
	}
	void Destroy()
	{
		if (m_bInitialized)
		{
			DeleteCriticalSection(&m_cs);
			ZeroMemory(&m_cs, sizeof(m_cs));
			m_bInitialized = false;
		}
	}
	void Lock()
	{
		if (m_bInitialized)
			EnterCriticalSection(&m_cs);
	}
	BOOL TryLock()
	{
		if (m_bInitialized)
			TryEnterCriticalSection(&m_cs);
	}
	void Unlock()
	{
		if (m_bInitialized)
			LeaveCriticalSection(&m_cs);
	}
private:
	CRITICAL_SECTION m_cs;
	bool m_bInitialized;
};

class AVPSemaphore
{
public:
	AVPSemaphore() : m_hSemaphore(NULL), m_Count(0) { ZeroMemory(m_Name, sizeof(TCHAR)*MAX_PATH); }
	AVPSemaphore(LONG initVal, LONG maxVal = 1024) { Create(initVal, maxVal); }
	~AVPSemaphore() { Destroy(); }
	void Create(LONG initVal, LONG maxVal = 1024);
	void Destroy();
	BOOL Release();
	UINT GetCurrentCount() { if (m_hSemaphore) { return m_Count; } else return 0; }
	LPTSTR GetName() { if (m_hSemaphore) { return m_Name; } else return NULL; }
	DWORD WaitTimeout(DWORD timeout);
	DWORD TryWait()
	{
		return WaitTimeout(0);
	}

	DWORD Wait()
	{
		return WaitTimeout(INFINITE);
	}
private:
	HANDLE	m_hSemaphore;
	LONG	m_Count;
	TCHAR   m_Name[MAX_PATH];
};

class AVPConditionVariable
{
#if 1
public:
	AVPConditionVariable() : m_SleepCount(0), m_ReleasingCount(0) {}
	~AVPConditionVariable() {}
	BOOL Create();
	void Destroy();
	int Wake();
	int WakeAll();
	int SleepTimeout(AVPMutex* mutex, DWORD timeout);
	int Sleep(AVPMutex* mutex);
private:
	void Lock() { m_Mutex.Lock(); }
	void Unlock() { m_Mutex.Unlock(); }
	AVPMutex m_Mutex;
	AVPSemaphore m_SemSleep;
	AVPSemaphore m_SemSleepFinish;
	int m_SleepCount;
	int m_ReleasingCount;
#else
	// vista+ only
	//CRITICAL_SECTION CritSection;
	CONDITION_VARIABLE m_ConditionVar;
#endif
};

typedef DWORD(WINAPI *PFN_AVPTHREAD_PROC)(LPVOID param);

class AVPThread
{
public:
	AVPThread() : m_hThread(NULL), m_ThreadId(0), m_bSuspended(false) {}
	AVPThread(PFN_AVPTHREAD_PROC proc, LPVOID lpParameter, bool bCreateSuspended = false) :
		m_hThread(NULL), m_ThreadId(0), m_bSuspended(false)
	{
		Create(proc, lpParameter, bCreateSuspended);
	}
	
	~AVPThread() { if (m_hThread) { CloseHandle(m_hThread); m_hThread = NULL; } }
	HANDLE Create(PFN_AVPTHREAD_PROC proc, LPVOID lpParameter, bool bCreateSuspended = false)
	{
		m_bSuspended = bCreateSuspended;
		DWORD flag = m_bSuspended ? CREATE_SUSPENDED : 0;

		m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)proc, lpParameter, flag, &m_ThreadId);
		return m_hThread;
	}
	//void Terminate()
	//{
	//	if (m_hThread)
	//	{
	//		TerminateThread(m_hThread, 0);
	//		//CloseHandle(m_hThread);
	//		m_hThread = NULL;
	//		m_ThreadId = 0;
	//	}
	//}
	HANDLE GetHandle() { return m_hThread; }
	DWORD GetId() { return m_ThreadId; }
	void WaitClose()
	{
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
//		DWORD dd = GetLastError();
//		dd;
	}
	void Suspend() { if (!m_bSuspended) { SuspendThread(m_hThread); m_bSuspended = TRUE; } }
	void Resume() { if (m_bSuspended) { ResumeThread(m_hThread); m_bSuspended = FALSE; } }
	void SetPriority(int priority)
	{
		/*
		THREAD_PRIORITY_HIGHEST
		THREAD_PRIORITY_ABOVE_NORMAL
		THREAD_PRIORITY_NORMAL
		THREAD_PRIORITY_BELOW_NORMAL
		THREAD_PRIORITY_LOWEST
		*/
		SetThreadPriority(m_hThread, priority);
	}
	int GetPriority() { return GetThreadPriority(m_hThread); }
protected:
	bool m_bSuspended;
	HANDLE m_hThread;
	DWORD m_ThreadId;
};

#endif //_AVP_SYNC_H_
