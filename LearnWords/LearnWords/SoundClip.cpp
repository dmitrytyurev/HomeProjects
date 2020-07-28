#include "stdafx.h"
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "SoundClip.h"

//===============================================================================================
// 
//===============================================================================================

float SetWindowsAudioVolume(float newVolume)
{
	HRESULT hr = 0;

	CoInitialize(nullptr);
	IMMDeviceEnumerator *deviceEnumerator = nullptr;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	IMMDevice *defaultDevice = nullptr;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	deviceEnumerator->Release();
	deviceEnumerator = nullptr;

	IAudioEndpointVolume *endpointVolume = nullptr;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (LPVOID *)&endpointVolume);
	defaultDevice->Release();
	defaultDevice = nullptr;
	float prevVolume = 0;
	hr = endpointVolume->GetMasterVolumeLevelScalar(&prevVolume);
	hr = endpointVolume->SetMasterVolumeLevelScalar(newVolume, nullptr);
	endpointVolume->Release();

	CoUninitialize();
	return prevVolume;
}

//===============================================================================================
// 
//===============================================================================================

struct handle_data
{
	unsigned long process_id;
	HWND best_handle;
};

//===============================================================================================
// 
//===============================================================================================

BOOL is_main_window(HWND handle)
{
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

//===============================================================================================
// 
//===============================================================================================

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle)) {
		return TRUE;
	}
	data.best_handle = handle;
	return FALSE;
}

//===============================================================================================
// 
//===============================================================================================

HWND find_main_window(unsigned long process_id)
{
	handle_data data;
	data.process_id = process_id;
	data.best_handle = 0;
	EnumWindows(enum_windows_callback, (LPARAM)&data);
	return data.best_handle;
}

//===============================================================================================
// 
//===============================================================================================

SoundClip::SoundClip(): processHandle((HANDLE)-1), prevVolume(0), timeToTurnVolumeUp(0), fullTimeOfPlay(0), quitNow(false)
{
}


//===============================================================================================
// 
//===============================================================================================

void SoundClip::play(const std::string& fileNameWithPath, int startMilliSec, int stopMilliSec)
{
	stop();
	quitNow = false;
	prevVolume = SetWindowsAudioVolume(0);

	processHandle = (HANDLE)_spawnl(_P_NOWAIT,
		"C:\\Program Files\\VideoLAN\\VLC\\vlc.exe",
		"vlc.exe",
		fileNameWithPath.c_str(),
		"--start-time",
		std::to_string(startMilliSec / 1000).c_str(),
		"",
		NULL);

	while (true)
	{
		DWORD pid = GetProcessId(processHandle);
		if (pid == 0)
			continue;

		HWND hWnd = find_main_window(pid);
		if (!hWnd)
			continue;
		break;
	}

	timer.start(1000);
	timeToTurnVolumeUp = startMilliSec % 1000;
	fullTimeOfPlay = stopMilliSec - startMilliSec / 1000 * 1000; // Полное время звучания ролика с учётом округления до начала первой секунды

	threadObj = std::thread(&SoundClip::watch_thread, this);
}

//===============================================================================================
// 
//===============================================================================================

void SoundClip::stop_player()
{
	if (int(processHandle) == -1)
		return;

	DWORD pid = GetProcessId(processHandle);
	if (pid == 0)
		return;

	HWND hWnd = find_main_window(pid);
	if (!hWnd)
		return;

	::SendMessage(hWnd, WM_CLOSE, NULL, NULL);

	while (true)
	{
		DWORD res = 0;
		GetExitCodeProcess(processHandle, &res);
		if (res != STILL_ACTIVE)
			break;
		Sleep(3);
	}
	CloseHandle(processHandle);
	processHandle = (HANDLE)-1;
}

//===============================================================================================
// 
//===============================================================================================

void SoundClip::watch_thread()
{
	while (true)
	{
		if (quitNow)
		{
			stop_player();
			SetWindowsAudioVolume(prevVolume);
			return;
		}
		
		__int64 curTime = timer.get();
		if (curTime >= timeToTurnVolumeUp)
			break;
		Sleep(5);
	}

	SetWindowsAudioVolume(prevVolume);

	while (!quitNow)
	{
		__int64 curTime = timer.get();
		if (curTime >= fullTimeOfPlay)
			break;
		Sleep(5);
	}

	stop_player();
}

//===============================================================================================
// 
//===============================================================================================

void SoundClip::stop()
{
	quitNow = true;
	if (threadObj.joinable())
		threadObj.join();
	quitNow = false;
}

//===============================================================================================
// 
//===============================================================================================


SoundClip::~SoundClip()
{
	stop();
}

