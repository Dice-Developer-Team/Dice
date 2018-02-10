#include "..\CQSDK\CQAPI_EX.h"
#include "..\CQSDK\CQEVE.h"

#include <Windows.h>
#include <iostream>
#include <string>
#include <list>
#include <DbgHelp.h>
#pragma comment( lib, "dbghelp.lib" )
using namespace std;

struct stack {
	string name, file; DWORD line;
	stack(string name, string file, DWORD line):name(name),file(file),line(line){}
	string tostring() {
		return string().append(file).append(": ").append(name).append(": ").append(to_string(line));
	}
};
list<stack> dump_callstack(CONTEXT *context)
{
	list<stack> r;
	STACKFRAME sf;
	memset(&sf, 0, sizeof(STACKFRAME));

	sf.AddrPC.Offset = context->Eip;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Offset = context->Esp;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = context->Ebp;
	sf.AddrFrame.Mode = AddrModeFlat;

	DWORD machineType = IMAGE_FILE_MACHINE_I386;

	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();

	while (
		StackWalk(machineType, hProcess, hThread, &sf, context, 0, SymFunctionTableAccess, SymGetModuleBase, 0)
		&&
		sf.AddrFrame.Offset
		)
	{
		string name, file; DWORD line;

		//函数名字
		BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 1024];
		PIMAGEHLP_SYMBOL ps = (PIMAGEHLP_SYMBOL)symbolBuffer;
		ps->SizeOfStruct = sizeof(symbolBuffer);
		ps->MaxNameLength = 1024;

		if (SymGetSymFromAddr(hProcess, sf.AddrPC.Offset, 0, ps))
			name = ps->Name;
		else
			name = "unknown";

		//文件地址
		IMAGEHLP_LINE lineInfo = { sizeof(IMAGEHLP_LINE) };
		DWORD dwLineDisplacement;

		if (SymGetLineFromAddr(hProcess, sf.AddrPC.Offset, &dwLineDisplacement, &lineInfo))
		{
			file = lineInfo.FileName;
			line = lineInfo.LineNumber;
#ifdef _DEBUG
			//--line;
#endif // _DEBUG

		}
		else { file = "unknown"; line = 0; }

		//输出
		//if (!strcmp(file.c_str(), "f:\\dd\\vctools\\crt\\vcstartup\\src\\startup\\exe_common.inl") && !strcmp(name.c_str(), "invoke_main") && line == 63)break;

		//printf("%s: %s(): %u\n",  file.c_str(), name.c_str(), line);
		//r.append(file).append(": ").append(name).append(": ").append(to_string(line)).append("\r\n");
		r.push_back(stack(name,file,line));
	}
	return r;
}
#include "..\CQSDK\CQconstant.h"
int dump(EXCEPTION_POINTERS* ep,char*evename,char*msg) {
	if (SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
		auto stack=dump_callstack(ep->ContextRecord);
		CQ::addLog(Log_Debug, evename, msg);
		for (auto s : stack) 
			CQ::addLog(Log_Debug, evename, s.tostring().c_str());
		SymCleanup(GetCurrentProcess());
	}
	return EXCEPTION_EXECUTE_HANDLER;
}