/*
    This file is part of the KDE games library
    Copyright (C) 2001 Nicolas Hadacek (hadacek@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "ghighscores_item.h"

#include <khighscore.h>
#include <kglobal.h>

#include "ghighscores_internal.h"
#include "ghighscores.h"


namespace KExtHighscores
{

//-----------------------------------------------------------------------------
Item::Item(const QVariant &def, const QString &label, int alignment)
    : _default(def), _label(label), _alignment(alignment),
      _format(NoFormat), _special(NoSpecial)
{}

QVariant Item::read(uint, const QVariant &value) const
{
    return value;
}

void Item::setPrettyFormat(Format format)
{
    bool buint = ( _default.type()==QVariant::UInt );
    bool bdouble = ( _default.type()==QVariant::Double );
    bool bnum = ( buint || bdouble || _default.type()==QVariant::Int );

    switch (format) {
    case OneDecimal:
    case Percentage:
        Q_ASSERT(bdouble);
        break;
    case MinuteTime:
        Q_ASSERT(bnum);
        break;
    case DateTime:
    	Q_ASSERT( _default.type()==QVariant::DateTime );
	break;
    case NoFormat:
        break;
    }

    _format = format;
}

void Item::setPrettySpecial(Special special)
{
    bool buint = ( _default.type()==QVariant::UInt );
    bool bdouble = ( _default.type()==QVariant::Double );
    bool bnum = ( buint || bdouble || _default.type()==QVariant::Int );

    switch (special) {
    case ZeroNotDefined:
        Q_ASSERT(bnum);
        break;
    case NegativeNotDefined:
        Q_ASSERT(bnum && !buint);
        break;
    case Anonymous:
        Q_ASSERT( _default.type()==QVariant::String );
        break;
    case NoSpecial:
        break;
    }

     _special = special;
}

QString Item::timeFormat(uint n)
{
    Q_ASSERT( n<3600 && n!=0 );
    n = 3600 - n;
    return QString::number(n / 60).rightJustify(2, '0') + ':'
        + QString::number(n % 60).rightJustify(2, '0');
}

QString Item::pretty(uint, const QVariant &value) const
{
    switch (_special) {
    case ZeroNotDefined:
        if ( value.toUInt()==0 ) return "--";
        break;
    case NegativeNotDefined:
        if ( value.toInt()<0 ) return "--";
        break;
    case Anonymous:
        if ( value.toString()==ItemContainer::ANONYMOUS )
            return i18n("anonymous");
        break;
    case NoFormat:
        break;
    }

    switch (_format) {
    case OneDecimal:
        return QString::number(value.toDouble(), 'f', 1);
    case Percentage:
        return QString::number(value.toDouble(), 'f', 1) + "%";
    case MinuteTime:
        return timeFormat(value.toUInt());
    case DateTime:
        if ( value.toDateTime().isNull() ) return "--";
        return KGlobal::locale()->formatDateTime(value.toDateTime());
    case NoSpecial:
        break;
    }

    return value.toString();
}

//-----------------------------------------------------------------------------
ScoreItem::ScoreItem()
    : Item((uint)0, i18n("Score"), Qt::AlignRight)
{}

MeanScoreItem::MeanScoreItem()
    : Item((double)0, i18n("Mean score"), Qt::AlignRight)
{
    setPrettyFormat(OneDecimal);
    setPrettySpecial(ZeroNotDefined);
}

BestScoreItem::BestScoreItem()
    : Item((uint)0, i18n("Best score"), Qt::AlignRight)
{
    setPrettySpecial(ZeroNotDefined);
}

//-----------------------------------------------------------------------------
DataArray::DataArray(const ItemArray &items)
    : _items(items)
{
    _data.resize( _items.size() );
    for (uint i=0; i<_data.size(); i++)
        _data[i] = _items[i]->item()->defaultValue();
}

DataArray &DataArray::operator =(const DataArray &array)
{
    _data = array._data;
    return *this;
}

void DataArray::setData(const QString &name, const QVariant &value)
{
    Q_ASSERT( _data.size()==_items.size() );
    int i = _items.findIndex(name);
    Q_ASSERT( i!=-1);
    Q_ASSERT( value.type()==_data[i].type() );
    _data[i] = value;
}

const QVariant &DataArray::data(const QString &name) const
{
    Q_ASSERT( _data.size()==_items.size() );
    int i = _items.findIndex(name);
    Q_ASSERT( i!=-1 );
    return _data[i];
}

void DataArray::read(uint k)
{
    Q_ASSERT( _data.size()==_items.size() );
    for (uint i=0; i<_data.size(); i++) {
        if ( !_items[i]->isStored() ) continue;
        _data[i] = _items[i]->read(k);
    }
}

void DataArray::write(uint k, uint nb) const
{
    Q_ASSERT( _data.size()==_items.size() );
    for (uint i=0; i<_data.size(); i++) {
        const ItemContainer *item = _items[i];
        if ( !item->isStored() ) continue;
        for (uint j=nb-1; j>k; j--)  item->write(j, item->read(j-1));
        item->write(k, _data[i]);
    }
}

QDataStream &operator <<(QDataStream &s, const DataArray &array)
{
    s << array._data;
    return s;
}

QDataStream &operator >>(QDataStream &s, DataArray &array)
{
    s >> array._data;
    return s;
}

//-----------------------------------------------------------------------------
Score::Score(ScoreType type)
    : DataArray(kHighscores->scoreInfos()), _type(type)
{}

bool Score::operator <(const Score &score) const
{
    return kHighscores->isStrictlyWorse(*this, score);
}

QDataStream &operator <<(QDataStream &s, const Score &score)
{
    s << (Q_UINT8)score.type();
    s << (const DataArray &)score;
    return s;
}

QDataStream &operator >>(QDataStream &s, Score &score)
{
    Q_UINT8 type;
    s >> type;
    score._type = (ScoreType)type;
    s >> (DataArray &)score;
    return s;
}

}; // namespace