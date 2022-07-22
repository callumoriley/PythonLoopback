// Reference for capturing audio from a Windows audio stream: https://docs.microsoft.com/en-us/windows/win32/coreaudio/capturing-a-stream
// Used this working example to get started: https://github.com/NeelOommen/WASAPI-LoopBack
// Used this guide to create the bindings between Python and C++: https://docs.microsoft.com/en-us/visualstudio/python/working-with-c-cpp-python-in-visual-studio?view=vs-2022

#pragma comment( lib, "Ole32.lib" )

#include <Python.h>
#include <iostream>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <winerror.h>
#include <math.h>
#include <objbase.h>
#include <functiondiscoverykeys.h>

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000
// SLEEP_DURATION in milliseconds. Logic is that human hearing only goes down to 20 Hz, so in a period of 20 Hz
// the peak of any audible should appear at least once. 1/20Hz = 0.050 s = 50 ms
#define SLEEP_DURATION 50

#define SAFE_RELEASE(punk) \
	if((punk) != NULL) \
		{(punk)->Release(); (punk) = NULL;}

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

PyObject* getCurrentAmplitude(void) {
    CoInitialize(nullptr);
    HRESULT hr; // gives results of stuff (errors mostly)
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL; // IMMDevice interface for rendering endpoint device
    IAudioClient* pAudioClient = NULL; // these guys work together to handle the audio stream
    IAudioCaptureClient* pCaptureClient = NULL; // this is the stream object I think, where is it configured as loopback
    WAVEFORMATEX* pwfx = NULL; // contains format information (wFormatTag says that you need to get the rest of the information from the extensible struct)
    UINT32 packetLength = 0;
    BOOL bDone = FALSE;
    BYTE* pData;
    DWORD flags;

    float currentMax = 0.0; // calculate the RMS amplitude of the current frame
    float currentAmplitude;

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice); // this is where it should be configured as loopback
    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    hr = pAudioClient->GetMixFormat(&pwfx);
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, NULL);
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    hr = pAudioClient->Start();

    Sleep(SLEEP_DURATION);

    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);

    if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && numFramesAvailable != 0) {
        for (int i = 0; i < numFramesAvailable; i += 8) {
            currentAmplitude = ((*(float*)(pData + i)) + (*(float*)(pData + i + 4))) / 2;
            // this uses both channels, the first channel audio is stored in the first 4 bytes (32 bits) and the second channel audio is stored in the next 4 bytes (8 bytes, 64 bits total)
            // can cast a pointer to the start of float bytes to a float pointer before dereferencing it to convert the bytes into a float

            if (currentAmplitude > currentMax) currentMax = currentAmplitude;
        }
    }

    hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    hr = pAudioClient->Stop();  // Stop recording.

    CoUninitialize();

Exit:
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator);
    SAFE_RELEASE(pDevice);
    SAFE_RELEASE(pAudioClient);
    SAFE_RELEASE(pCaptureClient);
    CoUninitialize();

    return PyFloat_FromDouble((double)currentMax);
}
static PyMethodDef PythonLoopback_methods[] = {
    {"get_current_amplitude", (PyCFunction)getCurrentAmplitude, METH_NOARGS, nullptr},
    {nullptr, nullptr, 0, nullptr}

};
static PyModuleDef PythonLoopback_module = {
    PyModuleDef_HEAD_INIT,
    "PythonLoopback",
    "Allows Python to get information about audio currently playing on the system",
    0,
    PythonLoopback_methods
};
PyMODINIT_FUNC PyInit_PythonLoopback() {
    return PyModule_Create(&PythonLoopback_module);
}