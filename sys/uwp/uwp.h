/* NetHack 3.6	uwp.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#define _USE_MATH_DEFINES

#include <windows.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector> 
#include <streambuf>
#include <sstream>
#include <setjmp.h>

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>
#include <collection.h>
#include <ppltasks.h>

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>

#include <agile.h>
#include <concrt.h>
#include <math.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <dwrite_2.h>
#include <wincodec.h>
#include <DirectXMath.h>
#include <agile.h>
#include <d3d11_3.h>
#include <dxgi1_4.h>
#include <d2d1_3.h>
#include <wrl.h>
#include <wrl/client.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>

#include "uwputil.h"
#include "uwpasciitexture.h"
#include "uwptextgrid.h"
#include "uwpoption.h"
#include "uwpfilehandler.h"
#include "uwpeventqueue.h"
#include "uwpdeviceResources.h"
#include "uwpsteptimer.h"
#include "uwpnethackmain.h"
#include "uwpapp.h"
#include "uwpfont.h"
#include "uwpdxhelper.h"
#include "uwpglobals.h"

extern"C" {
    #include "hack.h"
    #include "winuwp.h"
    #include "spell.h"

    #include "date.h"
    #include "patchlevel.h"
    #include "dlb.h"
    #include "func_tab.h"
    #include "tcap.h"

    extern char MapScanCode(const Nethack::Event & e);
    extern int raw_getchar();

    void uwp_play_nethack(void);
    void uwp_main(std::wstring & localDirW, std::wstring & installDirW);
    void uwp_one_time_init(std::wstring & localDirW, std::wstring & installDirW);
    void uwp_init_options();
    void uwp_warn(const char * s, ...);
    void uwp_error(const char * s, ...);

}
