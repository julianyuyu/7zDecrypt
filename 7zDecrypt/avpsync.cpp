
#include "pch.h"
#include "avpsync.h"

/* Create a semaphore */

void AVPSemaphore::Create(LONG initVal, LONG maxVal /*= 1024 */)
{
	m_hSemaphore = CreateSemaphore(NULL, initVal, maxVal, m_Name);
	m_Count = initVal;
}

void AVPSemaphore::Destroy()
{
	if (m_hSemaphore)
	{
		CloseHandle(m_hSemaphore);
		m_hSemaphore = NULL;
		m_Count = 0;
	}
}

DWORD AVPSemaphore::WaitTimeout(DWORD timeout)
{
	if (m_hSemaphore)
	{
		WAIT_FAILED;
	}
	DWORD retval = WaitForSingleObject(m_hSemaphore, timeout);

	if (timeout != 0 && retval == WAIT_OBJECT_0)
	{
		InterlockedDecrement(&m_Count);
	}
	return retval;
}

BOOL AVPSemaphore::Release()
{
	if (!m_hSemaphore)
	{
		return FALSE;
	}
	// inc counter first, because after a successful release the semaphore may
	// immediately get destroyed by another thread which is waiting for this semaphore.
	InterlockedIncrement(&m_Count);

	if (ReleaseSemaphore(m_hSemaphore, 1, NULL) == FALSE)
	{
		InterlockedDecrement(&m_Count);
		return FALSE;
	}
	return TRUE;
}

/* Create a condition variable */
BOOL AVPConditionVariable::Create()
{
	BOOL ret = m_Mutex.Create();
	if (ret)
	{
		m_SemSleep.Create(0);
	}

	m_SemSleepFinish.Create(0);
	m_SleepCount = 0;
	m_ReleasingCount = 0;
	return ret;
}

/* Destroy a condition variable */
void AVPConditionVariable::Destroy()
{
	m_SemSleep.Destroy();
	m_SemSleepFinish.Destroy();
	m_Mutex.Destroy();
}

/* Restart one of the threads that are waiting on the condition variable */
// be signaled
int AVPConditionVariable::Wake()
{
	/* If there are waiting threads not already signaled, then
	signal the condition and wait for the thread to respond.
	*/
	Lock();
	if (m_SleepCount > m_ReleasingCount)
	{
		m_ReleasingCount++;
		m_SemSleep.Release();
		Unlock();
		m_SemSleepFinish.Wait();
	}
	else
	{
		Unlock();
	}

	return 0;
}

/* Restart all threads that are waiting on the condition variable */
int AVPConditionVariable::WakeAll()
{

	/* If there are waiting threads not already signaled, then
	signal the condition and wait for the thread to respond.
	*/
	Lock();

	if (m_SleepCount > m_ReleasingCount)
	{
		int WaitingCount = (m_SleepCount - m_ReleasingCount);
		m_ReleasingCount = m_SleepCount;
		for (int i = 0; i < WaitingCount; ++i)
		{
			m_SemSleep.Release();
		}
		/* Now all released threads are blocked here, waiting for us.
		Collect them all (and win fabulous prizes!) :-)
		*/
		Unlock();
		for (int i = 0; i < WaitingCount; ++i)
		{
			m_SemSleepFinish.Wait();
		}
	}
	else
	{
		Unlock();
	}

	return 0;
}

/* Wait on the condition variable for at most 'ms' milliseconds.
The mutex must be locked before entering this function!
The mutex is unlocked during the wait, and locked again after the wait.

Typical use:

Thread A:
LockMutex(lock);
while ( ! condition ) {
CondWait(cond, lock);
}
UnlockMutex(lock);

Thread B:
LockMutex(lock);
...
condition = true;
...
CondSignal(cond);
UnlockMutex(lock);
*/
int AVPConditionVariable::SleepTimeout(AVPMutex* mutex, DWORD timeout /*ms*/)
{
	int retval;

	/* Obtain the protection mutex, and increment the number of waiters.
	This allows the signal mechanism to only perform a signal if there
	are waiting threads.
	*/
	Lock();
	m_SleepCount++;
	Unlock();

	/* Unlock the mutex, as is required by condition variable semantics */
	mutex->Unlock();

	/* Wait for a signal */
	retval = m_SemSleep.WaitTimeout(timeout);

	/* Let the signaler know we have completed the wait, otherwise
	the signaler can race ahead and get the condition semaphore
	if we are stopped between the mutex unlock and semaphore wait,
	giving a deadlock.  See the following URL for details:
	http://web.archive.org/web/20010914175514/http://www-classic.be.com/aboutbe/benewsletter/volume_III/Issue40.html#Workshop
	*/
	Lock();
	if (m_ReleasingCount > 0)
	{
		/* If we timed out, we need to eat a condition signal */
		if (retval > 0)
		{
			m_SemSleep.Wait();
		}
		/* We always notify the signal thread that we are done */

		m_SemSleepFinish.Release();
		/* Signal handshake complete */
		m_ReleasingCount--;
	}
	m_SleepCount--;
	Unlock();

	/* Lock the mutex, as is required by condition variable semantics */
	mutex->Lock();
	return retval;
}

/* Wait on the condition variable forever */
int AVPConditionVariable::Sleep(AVPMutex* mutex)
{
	return SleepTimeout(mutex, INFINITE);
}
