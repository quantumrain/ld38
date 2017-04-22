#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#pragma warning(disable:4091) // warning C4091: 'typedef ': ignored on left of '' when no variable is declared

#include <windows.h>
#include <guiddef.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <xinput.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <dbt.h>
#include <versionhelpers.h>

#undef max
#undef min
#undef near
#undef far
#undef NEAR
#undef FAR
