#pragma once
#include <atlsync.h>
#define MAX_QUEUESIZE 20
typedef struct tag_Node
{
	void* data;
	int framecnt;
}Node, * lpNode;

typedef struct tag_FrameQueue
{
	Node* pNode[MAX_QUEUESIZE];
	int begin;
	int end;
	int iCount;
}FrameQueue;

class CFrameQueue
{
public:

	CFrameQueue() {
		lpFrameQueue = new FrameQueue();
		lpFrameQueue->iCount = 0;
		lpFrameQueue->begin = 0;
		lpFrameQueue->end = 0;
		InitializeCriticalSection(&locker);
	}
	~CFrameQueue() {
		DeleteCriticalSection(&locker);
		delete lpFrameQueue;
	}

	BOOL push(Node*& pNode)
	{
		ATL::CCritSecLock lock(locker);
		int cnt = (lpFrameQueue->end + 1) % MAX_QUEUESIZE;
		if (cnt == lpFrameQueue->begin)
			return FALSE;
		lpFrameQueue->pNode[cnt] = pNode;
		lpFrameQueue->iCount++;
		lpFrameQueue->end = cnt;
		return TRUE;
	}

	BOOL pop(Node*& pNode)
	{
		ATL::CCritSecLock lock(locker,TRUE);
		if (lpFrameQueue->begin == lpFrameQueue->end)
			return FALSE;
		int cnt = (lpFrameQueue->begin + 1) % MAX_QUEUESIZE;
		pNode = lpFrameQueue->pNode[cnt];
		lpFrameQueue->iCount--;
		lpFrameQueue->begin = cnt;
		return TRUE;
	}

	int getSize()
	{
		ATL::CCritSecLock lock(locker);
		return lpFrameQueue->iCount;
	}
	BOOL isEmpty()
	{
		ATL::CCritSecLock lock(locker);
		if (lpFrameQueue->end == lpFrameQueue->begin)
			return TRUE;
		return FALSE;
	}

	BOOL Clear()
	{
		Node* pNode = NULL;
		while (lpFrameQueue->iCount != 0)
			pop(pNode);
		return 1;
	}
private:
	CRITICAL_SECTION locker;
	FrameQueue* lpFrameQueue;
};

