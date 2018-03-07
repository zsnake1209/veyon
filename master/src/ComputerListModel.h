/*
 * ComputerListModel.h - data model for computer objects
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef COMPUTER_LIST_MODEL_H
#define COMPUTER_LIST_MODEL_H

#include <QAbstractListModel>
#include <QImage>

#include "ComputerControlInterface.h"

class ComputerManager;

class ComputerListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	ComputerListModel( ComputerManager& manager,
					   const FeatureList& masterFeatures,
					   QObject *parent = nullptr );

	int rowCount( const QModelIndex& parent = QModelIndex() ) const override;

	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;

	ComputerControlInterface::Pointer computerControlInterface( const QModelIndex& index );

private slots:
	void beginInsertComputer( int index );
	void endInsertComputer();

	void beginRemoveComputer( int index );
	void endRemoveComputer();

	void reload();

	void updateComputerScreen( int index );

private:
	void loadIcons();
	QImage prepareIcon( const QImage& icon );
	QImage computerDecorationRole( ComputerControlInterface::Pointer controlInterface ) const;
	QString computerToolTipRole( ComputerControlInterface::Pointer controlInterface ) const;
	static QString computerDisplayRole( ComputerControlInterface::Pointer controlInterface );
	static QString computerStateDescription( ComputerControlInterface::Pointer controlInterface );
	static QString loggedOnUserInformation( ComputerControlInterface::Pointer controlInterface );
	QString activeFeatures(  ComputerControlInterface::Pointer controlInterface ) const;

	ComputerManager& m_manager;
	const FeatureList m_masterFeatures;
	QImage m_iconDefault;
	QImage m_iconConnectionProblem;
	QImage m_iconDemoMode;

};

#endif // COMPUTER_LIST_MODEL_H
