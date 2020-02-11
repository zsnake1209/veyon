/*
 * LinuxInputDeviceFunctions.cpp - implementation of LinuxInputDeviceFunctions class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include "PlatformServiceFunctions.h"
#include "LinuxInputDeviceFunctions.h"
#include "LinuxKeyboardShortcutTrapper.h"

#include <X11/XKBlib.h>


void LinuxInputDeviceFunctions::enableInputDevices()
{
	if( m_inputDevicesDisabled )
	{
		restoreKeyMapTable();

		m_inputDevicesDisabled = false;
	}
}



void LinuxInputDeviceFunctions::disableInputDevices()
{
	if( m_inputDevicesDisabled == false )
	{
		setEmptyKeyMapTable();

		m_inputDevicesDisabled = true;
	}
}



KeyboardShortcutTrapper* LinuxInputDeviceFunctions::createKeyboardShortcutTrapper( QObject* parent )
{
	return new LinuxKeyboardShortcutTrapper( parent );
}



void LinuxInputDeviceFunctions::injectKeyEvent( LinuxInputDeviceFunctions::KeySym keySym, bool down )
{
	if( down )
	{
		m_keyboardInput.pressKey( keySym );
	}
	else
	{
		m_keyboardInput.releaseKey( keySym );
	}
}



void LinuxInputDeviceFunctions::setEmptyKeyMapTable()
{
	if( m_origKeyTable )
	{
		XFree( m_origKeyTable );
	}

	auto display = XOpenDisplay( nullptr );
	XDisplayKeycodes( display, &m_keyCodeMin, &m_keyCodeMax );
	m_keyCodeCount = m_keyCodeMax - m_keyCodeMin;

	m_origKeyTable = XGetKeyboardMapping( display, ::KeyCode( m_keyCodeMin ), m_keyCodeCount, &m_keySymsPerKeyCode );

	auto newKeyTable = XGetKeyboardMapping( display, ::KeyCode( m_keyCodeMin ), m_keyCodeCount, &m_keySymsPerKeyCode );

	for( int i = 0; i < m_keyCodeCount * m_keySymsPerKeyCode; i++ )
	{
		newKeyTable[i] = 0;
	}

	XChangeKeyboardMapping( display, m_keyCodeMin, m_keySymsPerKeyCode, newKeyTable, m_keyCodeCount );
	XFlush( display );
	XFree( newKeyTable );
	XCloseDisplay( display );
}



void LinuxInputDeviceFunctions::restoreKeyMapTable()
{
	Display* display = XOpenDisplay( nullptr );

	XChangeKeyboardMapping( display, m_keyCodeMin, m_keySymsPerKeyCode,
							static_cast<::KeySym *>( m_origKeyTable ), m_keyCodeCount );

	XFlush( display );
	XCloseDisplay( display );

	XFree( m_origKeyTable );
	m_origKeyTable = nullptr;
}
