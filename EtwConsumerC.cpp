// EtwConsumerC.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "TraceSession.h"

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

TraceSession traceSession = TraceSession();

GUID StringToGuid(const std::string& str)
{
    GUID guid;
    sscanf_s(str.c_str(),
        "{%8x-%4hx-%4hx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx}",
        &guid.Data1, &guid.Data2, &guid.Data3,
        &guid.Data4[0], &guid.Data4[1], &guid.Data4[2], &guid.Data4[3],
        &guid.Data4[4], &guid.Data4[5], &guid.Data4[6], &guid.Data4[7]);

    return guid;
}

//HANDLE event = CreateEventA(NULL, true, false, LPCSTR("WaitForCallback"));

struct ITraceConsumer {
    virtual void OnEventRecord(PEVENT_RECORD eventPointer) {
		printf("callback called \n");
		traceSession.Close();
		//SetEvent(event);
    };
};

TraceSession::TraceSession() : _szSessionName(_tcsdup(LPCTSTR("AD Core")))
{
	std::cout<<"new session****************** \n";
}

TraceSession::~TraceSession(void)
{
    delete[]_szSessionName;
    delete _pSessionProperties;
}

bool TraceSession::Start()
{
	 std::cout << "starting!\n";
    if (!_pSessionProperties) {
		std::cout<<"init session prop \n";
        const size_t buffSize = sizeof(EVENT_TRACE_PROPERTIES) + (_tcslen(_szSessionName) + 1) * sizeof(TCHAR);
        _pSessionProperties = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(malloc(buffSize));
        ZeroMemory(_pSessionProperties, buffSize);
        _pSessionProperties->Wnode.BufferSize = buffSize;
        _pSessionProperties->Wnode.ClientContext = 1;
        _pSessionProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
        _pSessionProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    }
	std::cout<<"StartingTrace\n";
    // Create the trace session.
    _status = StartTrace(&hSession, _szSessionName, _pSessionProperties);
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::EnableProvider(const GUID& providerId, UCHAR level, ULONGLONG anyKeyword, ULONGLONG allKeyword)
{
	std::cout << "Enabling Provider\n";
    _status = EnableTrace(EVENT_CONTROL_CODE_ENABLE_PROVIDER, anyKeyword, level, &providerId, hSession);
		std::cout << _status<<" is Provider status\n";
    return (_status == ERROR_SUCCESS);
}

namespace
{

    VOID WINAPI EventRecordCallback(_In_ PEVENT_RECORD pEventRecord)
    {
        reinterpret_cast<ITraceConsumer*>(pEventRecord->UserContext)->OnEventRecord(pEventRecord);
    }

}

bool TraceSession::OpenTrace(ITraceConsumer* pConsumer)
{
    if (!pConsumer)
        return false;

    ZeroMemory(&_logFile, sizeof(EVENT_TRACE_LOGFILE));
    _logFile.LoggerName = _szSessionName;
    _logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    _logFile.EventRecordCallback = &EventRecordCallback;
    _logFile.Context = pConsumer;

    _hTrace = ::OpenTrace(&_logFile);
    return (_hTrace != 0);
}



bool TraceSession::Process()
{
    _status = ProcessTrace(&_hTrace, 1, NULL, NULL);
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::CloseTrace()
{
    _status = ::CloseTrace(_hTrace);
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::DisableProvider(const GUID& providerId)
{
	                      
    _status = EnableTrace(EVENT_CONTROL_CODE_DISABLE_PROVIDER, 0, 0, &providerId, hSession);
    return (_status == ERROR_SUCCESS);
}

bool TraceSession::Stop()
{
    _status = ControlTrace(hSession, _szSessionName, _pSessionProperties, EVENT_TRACE_CONTROL_STOP);
    delete _pSessionProperties;
    _pSessionProperties = NULL;

    return (_status == ERROR_SUCCESS);
}

ULONG TraceSession::Status() const
{
    return _status;
}

LONGLONG TraceSession::PerfFreq() const
{
    return _logFile.LogfileHeader.PerfFreq.QuadPart;
}

void TraceSession::Close() 
{
	CloseTrace();
	DisableProvider(StringToGuid("{DAF0B914-9C1C-450A-81B2-FEA7244F6FFA}"));
	Stop();
}

int Consume()
{
    ITraceConsumer consumer = ITraceConsumer();
    std::cout << "trace consumer was created!\n";

	
	if (traceSession.Start()) {
		std::cout << "trace session was started!  \n";
	}
	else {

		std::cout << "trace session didnt start  \n";
		ULONG status = traceSession.Status();
		std::cout << "trace session status is: " << status << "\n";

		if (status == ERROR_ALREADY_EXISTS) {
			if (traceSession.Stop()) {
				std::cout << "trace session was stopped!  \n";
				if (traceSession.Start()) {
					std::cout << "trace session was started!  \n";
				}
				else {
					printf("error trace session didnt start");
					return 1;
				}
			}
		}
		else {
			return 1;
		}
	}

	std::cout << "trace session was started!  \n";
	traceSession.EnableProvider(StringToGuid("{DAF0B914-9C1C-450A-81B2-FEA7244F6FFA}"), TRACE_LEVEL_VERBOSE); // office word
	//traceSession.EnableProvider(StringToGuid("{1C83B2FC-C04F-11D1-8AFC-00C04FC21914}"), TRACE_LEVEL_VERBOSE); // Active directory service : core
	std::cout << "provider session was enabled!\n";

	traceSession.OpenTraceA(&consumer);
	std::cout << "trace session was opened!\n";
	bool b = traceSession.Process();
	std::cout << "trace session was proccessed!\n";
	return 0;
	}

int main()
{ 
	return Consume();
}