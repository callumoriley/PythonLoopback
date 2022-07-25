// Reference for capturing audio from a Windows audio stream: https://docs.microsoft.com/en-us/windows/win32/coreaudio/capturing-a-stream
// Used this working example to get started: https://github.com/NeelOommen/WASAPI-LoopBack
// Used this guide to create the bindings between Python and C++: https://docs.microsoft.com/en-us/visualstudio/python/working-with-c-cpp-python-in-visual-studio?view=vs-2022
// Used this guide to help me create Numpy arrays in C++: https://stuff.mit.edu/afs/sipb/project/python/src/python-numeric-22.0/doc/www.pfdubois.com/numpy/html2/numpy-13.html
// Also used this guide: https://numpy.org/devdocs/reference/c-api/array.html

#pragma comment( lib, "Ole32.lib" )

#include <Python.h>
#include <numpy/arrayobject.h>
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
#define SAMPLING_RATE 48000

#define EXIT_ON_ERROR(hres) \
	if (FAILED(hres)) { std::cout<<"\nError Exit Triggered. 0x"<< std::hex << hres; goto Exit; }
#define SAFE_RELEASE(punk) \
	if((punk) != NULL) \
		{(punk)->Release(); (punk) = NULL;}

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

// gets the current amplitude of the sound being played by the computer
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

    float currentMax = NULL; // NULL to return as NaN if there's a problem, set to 0.0 when it starts recording
    float currentAmplitude;

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    EXIT_ON_ERROR(hr);
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice); // this is where it should be configured as loopback
    EXIT_ON_ERROR(hr);
    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, NULL);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->Start();
    EXIT_ON_ERROR(hr);

    Sleep(SLEEP_DURATION);

    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    EXIT_ON_ERROR(hr);
    hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
    EXIT_ON_ERROR(hr);

    if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && numFramesAvailable != 0) {
        currentMax = 0.0;
        for (int i = 0; i < numFramesAvailable*8; i += 32) {
            currentAmplitude = ((*(float*)(pData + i)) + (*(float*)(pData + i + 4))) / 2;
            // this uses both channels, the first channel audio is stored in the first 4 bytes (32 bits) and the second channel audio is stored in the next 4 bytes (8 bytes, 64 bits total)
            // can cast a pointer to the start of float bytes to a float pointer before dereferencing it to convert the bytes into a float

            if (currentAmplitude > currentMax) currentMax = currentAmplitude;
        }
    }

    hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
    EXIT_ON_ERROR(hr);
    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->Stop();  // Stop recording.
    EXIT_ON_ERROR(hr);

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
// records a sound buffer for a specified number of seconds into a numpy array
PyObject* recordBuffer(PyObject *, PyObject* o) {
  
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

    int numSamples = 0; // current number of samples
    int maxNumSamples = SAMPLING_RATE*PyFloat_AsDouble(o); // would like to remove the macro and replace it with pwfx->nSamplesPerSec

    npy_intp dimensions[] = { maxNumSamples, 2 }; // need the first argument to be changed with the required size (second argument will always be the same)
    PyArrayObject* output_buffer = (PyArrayObject*)PyArray_SimpleNew(2, dimensions, NPY_INT);
    PyArray_FILLWBYTE(output_buffer, 0);

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    EXIT_ON_ERROR(hr);
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice); // this is where it should be configured as loopback
    EXIT_ON_ERROR(hr);
    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, NULL);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr);
    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    EXIT_ON_ERROR(hr);

    hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

    hr = pAudioClient->Start();
    EXIT_ON_ERROR(hr);

    while (numSamples < maxNumSamples)
    {
        // Sleep for half the buffer duration.
        Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);
        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        EXIT_ON_ERROR(hr);
        BOOL bufferFull = false;
        while (packetLength != 0 && !bufferFull)
        {
            // Get the available data in buffer
            hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
            EXIT_ON_ERROR(hr);
            // numFramesAvailable*8, += 32, and +4 works, but sounds grainy
            for (UINT32 i = 0; i < numFramesAvailable*8; i += 16) { // was originally incrementing by 8
                if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT))
                {
                    npy_intp position[] = { numSamples, 0 };
                    *((npy_int*)PyArray_GetPtr(output_buffer, position)) = (npy_int)(32767 * (*(float*)(pData + i)));
                    position[1] = 1;
                    *((npy_int*)PyArray_GetPtr(output_buffer, position)) = (npy_int)(32767 * (*(float*)(pData + i + 4)));
                }
                else { // write silence
                    npy_intp position[] = { numSamples, 0 };
                    *((npy_int*)PyArray_GetPtr(output_buffer, position)) = 0;
                    position[1] = 1;
                    *((npy_int*)PyArray_GetPtr(output_buffer, position)) = 0;
                }
                numSamples++;
                if (numSamples >= maxNumSamples) {
                    bufferFull = true;
                    break;
                }
            }

            hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
            EXIT_ON_ERROR(hr);
            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            EXIT_ON_ERROR(hr);
        }
    }

    hr = pAudioClient->Stop();
    EXIT_ON_ERROR(hr);

Exit:
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator);
    SAFE_RELEASE(pDevice);
    SAFE_RELEASE(pAudioClient);
    SAFE_RELEASE(pCaptureClient);
    CoUninitialize();

    return PyArray_Return(output_buffer); // return numpy array with sound buffer
}
static PyMethodDef PythonLoopback_methods[] = {
    {"get_current_amplitude", (PyCFunction)getCurrentAmplitude, METH_NOARGS, nullptr},
    {"record_buffer", (PyCFunction)recordBuffer, METH_O, nullptr},
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
    import_array(); // may not need
    return PyModule_Create(&PythonLoopback_module);
}