/*
 * SwimVncServer.cpp - implementation of SwimVncServer class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

extern "C" {
#include "rfb/rfb.h"
#include "rfb/rfbregion.h"
}

#include <QGuiApplication>
#include <QScreen>

#include "SwimVncServer.h"
#include "DeskDupEngine.h"
#include "PlatformInputDeviceFunctions.h"
#include "VeyonConfiguration.h"


struct SwimScreen
{
	~SwimScreen()
	{
		delete[] data;
	}

	rfbScreenInfoPtr rfbScreen{nullptr};
	QRgb* data{nullptr};
	QSize size;
	QPoint pointerPos{};
	int buttonMask{0};
	int bytesPerPixel{4};
};


static void handleClientGone( rfbClientPtr cl )
{
	vInfo() << cl->host;
}


static rfbNewClientAction handleNewClient( rfbClientPtr cl )
{
	cl->clientGoneHook = handleClientGone;

	vInfo() << "New client connection from host" << cl->host;

	return RFB_CLIENT_ACCEPT;
}


static void handleClipboardText( char* str, int len, rfbClientPtr cl )
{
	Q_UNUSED(cl)

	str[len] = '\0';
	qInfo() << "Got clipboard:" << str;
}


static void handleKeyEvent( rfbBool down, rfbKeySym keySym, rfbClientPtr cl )
{
	Q_UNUSED(cl)

	VeyonCore::platform().inputDeviceFunctions().injectKeyEvent( keySym, down );
}



static void handlePointerEvent( int buttonMask, int x, int y, rfbClientPtr cl )
{
	const auto screen = reinterpret_cast<SwimScreen *>( cl->screen->screenData );

	DWORD flags = MOUSEEVENTF_ABSOLUTE;

	if( x != screen->pointerPos.x() || y != screen->pointerPos.y() )
	{
		flags |= MOUSEEVENTF_MOVE;
	}

	if ( (buttonMask & rfbButton1Mask) != (screen->buttonMask & rfbButton1Mask) )
	{
		if( GetSystemMetrics(SM_SWAPBUTTON) )
		{
			flags |= (buttonMask & rfbButton1Mask) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
		}
		else
		{
			flags |= (buttonMask & rfbButton1Mask) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
		}
	}

	if( (buttonMask & rfbButton2Mask) != (screen->buttonMask & rfbButton2Mask) )
	{
		flags |= (buttonMask & rfbButton2Mask) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
	}

	if( (buttonMask & rfbButton3Mask) != (screen->buttonMask & rfbButton3Mask) )
	{
		if( GetSystemMetrics(SM_SWAPBUTTON) )
		{
			flags |= (buttonMask & rfbButton3Mask) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
		}
		else
		{
			flags |= (buttonMask & rfbButton3Mask) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
		}
	}

	int wheelDelta = 0;
	if( buttonMask & rfbWheelUpMask )
	{
		flags |= MOUSEEVENTF_WHEEL;
		wheelDelta = WHEEL_DELTA;
	}
	if( buttonMask & rfbWheelDownMask )
	{
		flags |= MOUSEEVENTF_WHEEL;
		wheelDelta = -WHEEL_DELTA;
	}

	// TODO: multi desktop support
	::mouse_event( flags,
				   DWORD(x * 65535 / screen->size.width()),
				   DWORD(y * 65535 / screen->size.height()),
				   DWORD(wheelDelta),
				   0 );

	screen->buttonMask = buttonMask;
	screen->pointerPos = { x, y };
}


SwimVncServer::SwimVncServer( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() ),
	m_engine( new DeskDupEngine )
{
}



void SwimVncServer::prepareServer()
{
	vDebug();
}



void SwimVncServer::runServer( int serverPort, const Password& password )
{
	while( true )
	{
		SwimScreen vncFrameBuffer;

		if( initFramebuffer( &vncFrameBuffer ) == false )
		{
			break;
		}

		initVncServer( serverPort, password, &vncFrameBuffer );

		bool idle = true;

		while( idle || readFramebuffer( &vncFrameBuffer ) )
		{
			rfbProcessEvents( vncFrameBuffer.rfbScreen, idle ? 100000 : 0 );

			if( vncFrameBuffer.rfbScreen->clientHead == nullptr )
			{
				idle = true;
			}

			for( auto clientPtr = vncFrameBuffer.rfbScreen->clientHead; clientPtr != nullptr; clientPtr = clientPtr->next )
			{
				if( sraRgnEmpty( clientPtr->requestedRegion ) == false )
				{
					idle = false;
					break;
				}
			}
		}

		delete[] reinterpret_cast<char **>( vncFrameBuffer.rfbScreen->authPasswdData );

		rfbScreenCleanup( vncFrameBuffer.rfbScreen );

		vDebug() << "Restarting";
	}
}



bool SwimVncServer::initVncServer( int serverPort, const VncServerPluginInterface::Password& password,
								  SwimScreen* vncFrameBuffer )
{
	vDebug();

	auto vncScreen = rfbGetScreen( nullptr, nullptr,
								   vncFrameBuffer->size.width(), vncFrameBuffer->size.height(),
								   8, 3, vncFrameBuffer->bytesPerPixel );

	if( vncScreen == nullptr )
	{
		return false;
	}

	vncScreen->desktopName = "SwimVNC";
	vncScreen->frameBuffer = reinterpret_cast<char *>( vncFrameBuffer->data );
	vncScreen->port = serverPort;
	vncScreen->kbdAddEvent = handleKeyEvent;
	vncScreen->ptrAddEvent = handlePointerEvent;
	vncScreen->newClientHook = handleNewClient;
	vncScreen->setXCutText = handleClipboardText;

	auto passwords = new char *[2];
	passwords[0] = qstrdup( password.toByteArray().constData() );
	passwords[1] = nullptr;
	vncScreen->authPasswdData = passwords;
	vncScreen->passwordCheck = rfbCheckPasswordByList;

	vncScreen->serverFormat.redShift = 16;
	vncScreen->serverFormat.greenShift = 8;
	vncScreen->serverFormat.blueShift = 0;

	vncScreen->serverFormat.redMax = 255;
	vncScreen->serverFormat.greenMax = 255;
	vncScreen->serverFormat.blueMax = 255;

	vncScreen->serverFormat.trueColour = true;
	vncScreen->serverFormat.bitsPerPixel = uint8_t(vncFrameBuffer->bytesPerPixel * 8);

	vncScreen->alwaysShared = true;
	vncScreen->handleEventsEagerly = true;
	vncScreen->deferUpdateTime = 5;

	vncScreen->screenData = vncFrameBuffer;

	rfbInitServer( vncScreen );

	rfbMarkRectAsModified( vncScreen, 0, 0, vncScreen->width, vncScreen->height );

	vncFrameBuffer->rfbScreen = vncScreen;

	return true;
}



bool SwimVncServer::initFramebuffer( SwimScreen* buffer )
{
	if( buffer->data == nullptr )
	{
		vInfo() << "initializing buffer";

		buffer->bytesPerPixel = QGuiApplication::primaryScreen()->depth() / 8;
		buffer->size = { QGuiApplication::primaryScreen()->geometry().width(),
						QGuiApplication::primaryScreen()->geometry().height() };
		m_engine->start( buffer->size.width(), buffer->size.height() );
		buffer->data = reinterpret_cast<QRgb *>( m_engine->getFramebuffer() );
	}

	return true;
}



static void handleChange( rfbScreenInfoPtr screen, const DeskDupEngine::ChangesRecord& changesRecord )
{
	const auto& rect = changesRecord.rect;

	switch( changesRecord.type )
	{
	case DeskDupEngine::SCREEN_SCREEN:
		// TODO
		break;
	case DeskDupEngine::SOLIDFILL:
	case DeskDupEngine::TEXTOUT:
	case DeskDupEngine::BLEND:
	case DeskDupEngine::TRANS:
	case DeskDupEngine::PLG:
	case DeskDupEngine::BLIT:;
		rfbMarkRectAsModified( screen, rect.left, rect.top, rect.right, rect.bottom );
		break;
	case DeskDupEngine::POINTERCHANGE:
		// TODO
		break;
	default:
		break;
	}
}



bool SwimVncServer::readFramebuffer( SwimScreen* buffer )
{
	QMutexLocker locker( &m_screenCapturerMutex );

	if( buffer->data == nullptr || buffer->rfbScreen == nullptr )
	{
		vInfo() << "Invalid buffer state";
		return false;
	}

	const auto previousCounter = m_engine->previousCounter();
	auto counter = m_engine->changesBuffer()->counter;

	if( previousCounter == counter ||
		counter < 1 ||
		counter > DeskDupEngine::MaxChanges )
	{
		return true;
	}

	const auto* changes = m_engine->changesBuffer()->changes;

	if( previousCounter < counter )
	{
		for( ULONG i = previousCounter+1; i <= counter; ++i )
		{
			handleChange( buffer->rfbScreen, changes[i] );
		}
	}
	else
	{
		for( ULONG i = previousCounter + 1; i < DeskDupEngine::MaxChanges; ++i )
		{
			handleChange( buffer->rfbScreen, changes[i] );
		}

		for( ULONG i = 1; i <= counter; ++i )
		{
			handleChange( buffer->rfbScreen, changes[i] );
		}
	}

	m_engine->setPreviousCounter( counter );

	return true;
}


IMPLEMENT_CONFIG_PROXY(SwimVncConfiguration)
