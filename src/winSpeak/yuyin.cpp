#include "sapi.h"
#include "sphelper.h"
#pragma comment(lib, "sapi.lib")

int yuyin()
{
	TCHAR szFileName[256] = L"D:\\~sys\\Desktop\\0.wav";//假设这里面保存着目标文件的路径
	USES_CONVERSION;
	WCHAR m_szWFileName[2048];
	wcscpy(m_szWFileName, T2W(szFileName));//转换成宽字符串

										   //创建一个输出流，绑定到wav文件
	CSpStreamFormat OriginalFmt;
	CComPtr<ISpStream>  cpWavStream;
	CComPtr<ISpStreamFormat>    cpOldStream;


	ISpVoice *pSpVoice;
	// 重要COM接口
	::CoInitialize(NULL);
	// COM初始化
	ISpStreamFormat *is;

	// 获取ISpVoice接口
	CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER, IID_ISpVoice, (void**)&pSpVoice);

	//pSpVoice->SetVolume(100);//设置音量



	HRESULT hr = pSpVoice->GetOutputStream(&cpOldStream);
	if (hr == S_OK)
		hr = OriginalFmt.AssignFormat(cpOldStream);
	else
		hr = E_FAIL;
	// 使用sphelper.h中提供的函数创建 wav 文件
	if (SUCCEEDED(hr))
	{
		hr = SPBindToFile(m_szWFileName, SPFM_CREATE_ALWAYS, &cpWavStream,
			&OriginalFmt.FormatId(), OriginalFmt.WaveFormatExPtr());
	}
	if (SUCCEEDED(hr))
	{
		//设置声音的输出到 wav 文件，而不是 speakers
		pSpVoice->SetOutput(cpWavStream, TRUE);
	}
	//开始朗读
	pSpVoice->Speak(L"我正在用软件播放声音", SPF_ASYNC | SPF_IS_NOT_XML, 0);

	//等待朗读结束
	pSpVoice->WaitUntilDone(INFINITE);
	cpWavStream.Release();

	//把输出重新定位到原来的流
	pSpVoice->SetOutput(cpOldStream, FALSE);

	return 0;
}