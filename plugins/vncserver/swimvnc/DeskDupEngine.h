/*
 * DeskDupEngine.h - declaration of DeskDupEngine class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
 *
 * The type definitions in the DeskDupEngine class are based on
 * UltraVNC's DeskDupEngine and ScreenCapture classes.
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


#pragma once

#include <windows.h>

#include "VeyonCore.h"

class DeskDupEngine
{
public:
	static constexpr auto SCREEN_SCREEN = 11;
	static constexpr auto BLIT = 12;
	static constexpr auto SOLIDFILL = 13;
	static constexpr auto BLEND = 14;
	static constexpr auto TRANS = 15;
	static constexpr auto PLG = 17;
	static constexpr auto TEXTOUT = 18;
	static constexpr auto POINTERCHANGE = 19;

	static constexpr auto MaxChanges = 2000;

	struct ChangesRecord
	{
		ULONG type;  // screen_to_screen, blit, newcache, oldcache
		RECT rect;
		POINT point;
	} ;

	struct ChangesBuffer
	{
		ULONG counter;
		ChangesRecord changes[MaxChanges];
	};

	DeskDupEngine();
	~DeskDupEngine();

	PCHAR getFramebuffer() const
	{
		return m_framebuffer;
	}

	ChangesBuffer* changesBuffer() const
	{
		return m_changesBuffer;
	}

	ULONG previousCounter() const
	{
		return m_previousCounter;
	}

	void setPreviousCounter( ULONG previousCounter )
	{
		m_previousCounter = previousCounter;
	}

	void setBlocked( bool blocked )
	{
		m_blocked = blocked;
	}

	bool isBlocked() const
	{
		return m_blocked;
	}

	void start( int w, int h );
	void stop();

	void hardwareCursor();
	void noHardwareCursor();

	void lock();
	void unlock();

	HANDLE screenEvent() const
	{
		return m_screenEvent;
	}

	HANDLE pointerEvent() const
	{
		return m_pointerEvent;
	}

private:
	typedef bool(*StartW8Fn)(bool);
	typedef bool (*StopW8Fn)();

	typedef void (*LockW8Fn)();
	typedef void (*UnlockW8Fn)();

	typedef void (*ShowCursorW8Fn)();
	typedef void (*HideCursorW8Fn)();

	PCHAR m_framebuffer{nullptr};
	ChangesBuffer* m_changesBuffer{nullptr};
	ULONG m_previousCounter{0};
	bool m_blocked{false};
	bool m_initialized{false};

	HMODULE m_module{nullptr};
	StartW8Fn m_startW8{nullptr};
	StopW8Fn m_stopW8{nullptr};

	LockW8Fn m_lockW8{nullptr};
	UnlockW8Fn m_unlockW8{nullptr};

	ShowCursorW8Fn m_showCursorW8{nullptr};
	HideCursorW8Fn m_hideCursorW8{nullptr};

	HANDLE m_fileMap{nullptr};
	LPVOID m_fileView{nullptr};

	HANDLE m_fileMapBitmap{nullptr};
	LPVOID m_fileViewBitmap{nullptr};
	HANDLE m_screenEvent{nullptr};
	HANDLE m_pointerEvent{nullptr};
};
