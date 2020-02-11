/*
 * DeskDupEngine.cpp - implementation of DeskDupEngine class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
 *
 * Some parts of the implementation are based on UltraVNC's
 * DeskDupEngine and ScreenCapture classes.
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "DeskDupEngine.h"

static const LPCTSTR IPCSharedMMF = "{3DA76AC7-62E7-44AF-A8D1-45022044BB3E}";
static const LPCTSTR IPCSharedMMFBitmap = "{0E3D996F-B070-4503-9090-198A9DA092D5}";
static const LPCTSTR IPCSharedEvent = "{3BFBA3A0-2133-48B5-B5BD-E58C72853FFB}";
static const LPCTSTR IPCSharedPointerEvent = "{3A77E11C-B0B4-40F9-BC8B-D249116A76FE}";


DeskDupEngine::DeskDupEngine()
{
#ifdef Q_OS_WIN64
	m_module = LoadLibrary( "ddengine64.dll" );
#else
	m_module = LoadLibrary( "ddengine.dll" );
#endif

	if( m_module )
	{
		m_startW8 = StartW8Fn( GetProcAddress(m_module, "StartW8") );
		m_stopW8 = StopW8Fn( GetProcAddress(m_module, "StopW8") );

		m_lockW8 = LockW8Fn( GetProcAddress(m_module, "LockW8") );
		m_unlockW8 = UnlockW8Fn( GetProcAddress(m_module, "UnlockW8") );

		m_showCursorW8 = ShowCursorW8Fn( GetProcAddress(m_module, "ShowCursorW8") );
		m_hideCursorW8 = HideCursorW8Fn( GetProcAddress(m_module, "HideCursorW8") );
	}

	m_initialized = m_startW8 && m_stopW8 && m_lockW8 && m_unlockW8 && m_showCursorW8 && m_hideCursorW8;
}



DeskDupEngine::~DeskDupEngine()
{
	stop();

	if( m_module )
	{
		FreeLibrary( m_module );
	}
}



void DeskDupEngine::start( int w, int h )
{
	m_previousCounter = 1;

	if( !m_initialized )
	{
		return;
	}

	const auto refw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	const auto refh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	if( refh == h && refw == w )
	{
		// prim or all monitors requested
		if( !m_startW8( false ) )
		{
			vWarning() << "DDengine failed, not supported by video driver";
			return;
		}
	}
	else if( !m_startW8(true) )
	{
		vWarning() << "DDengine failed, not supported by video driver";
		return;
	}

	if( m_fileMap != nullptr ||
		( m_fileMap = OpenFileMapping( FILE_MAP_READ | FILE_MAP_WRITE, FALSE, IPCSharedMMF ) ) == nullptr )
	{
		return;
	}

	m_fileView = MapViewOfFile( m_fileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0 );
	if( m_fileView == nullptr )
	{
		return;
	}

	m_changesBuffer = static_cast<ChangesBuffer *>( m_fileView );
	const auto size = m_changesBuffer->changes[0].rect.left;

	vDebug() << size;

	if( m_fileMapBitmap != nullptr ||
		( m_fileMapBitmap = OpenFileMapping( FILE_MAP_READ | FILE_MAP_WRITE, FALSE, IPCSharedMMFBitmap ) ) == nullptr )
	{
		return;
	}

	m_fileViewBitmap = MapViewOfFile( m_fileMapBitmap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0 );
	if( m_fileViewBitmap == nullptr )
	{
		return;
	}

	if( m_screenEvent != nullptr ||
		( m_screenEvent = OpenEvent( EVENT_ALL_ACCESS, false, IPCSharedEvent ) ) == nullptr )
	{
		return;
	}

	if( m_pointerEvent != nullptr ||
		( m_pointerEvent = OpenEvent( EVENT_ALL_ACCESS, false, IPCSharedPointerEvent ) ) == nullptr )
	{
		return;
	}

	m_framebuffer = PCHAR(m_fileViewBitmap);
}



void DeskDupEngine::stop()
{
	if( !m_initialized )
	{
		return;
	}

	if( m_fileView )
	{
		UnmapViewOfFile( m_fileView );
		m_fileView = nullptr;
	}

	if( m_fileMap )
	{
		CloseHandle( m_fileMap );
		m_fileMap = nullptr;
	}

	if( m_fileViewBitmap )
	{
		UnmapViewOfFile( m_fileViewBitmap );
		m_fileViewBitmap = nullptr;
	}

	if( m_fileMapBitmap )
	{
		CloseHandle( m_fileMapBitmap );
		m_fileMapBitmap = nullptr;
	}

	if( m_screenEvent )
	{
		CloseHandle( m_screenEvent );
		m_screenEvent = nullptr;
	}

	if( m_pointerEvent )
	{
		CloseHandle( m_pointerEvent );
		m_pointerEvent = nullptr;
	}

	m_stopW8();
}



void DeskDupEngine::lock()
{
	m_lockW8();
}



void DeskDupEngine::unlock()
{
	m_unlockW8();
}



void DeskDupEngine::hardwareCursor()
{
	m_showCursorW8();
}



void DeskDupEngine::noHardwareCursor()
{
	m_hideCursorW8();
}
