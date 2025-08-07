/****************************************************************************
** Meta object code from reading C++ file 'TournamentLeaderboardDialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../TournamentLeaderboardDialog.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TournamentLeaderboardDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_TournamentLeaderboardDialog_t {
    uint offsetsAndSizes[16];
    char stringdata0[28];
    char stringdata1[20];
    char stringdata2[1];
    char stringdata3[19];
    char stringdata4[16];
    char stringdata5[16];
    char stringdata6[20];
    char stringdata7[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_TournamentLeaderboardDialog_t::offsetsAndSizes) + ofs), len
Q_CONSTINIT static const qt_meta_stringdata_TournamentLeaderboardDialog_t qt_meta_stringdata_TournamentLeaderboardDialog = {
    {
        QT_MOC_LITERAL(0, 27),  // "TournamentLeaderboardDialog"
        QT_MOC_LITERAL(28, 19),  // "refreshLeaderboards"
        QT_MOC_LITERAL(48, 0),  // ""
        QT_MOC_LITERAL(49, 18),  // "exportCurrentImage"
        QT_MOC_LITERAL(68, 15),  // "applyCutClicked"
        QT_MOC_LITERAL(84, 15),  // "clearCutClicked"
        QT_MOC_LITERAL(100, 19),  // "cutLineScoreChanged"
        QT_MOC_LITERAL(120, 5)   // "value"
    },
    "TournamentLeaderboardDialog",
    "refreshLeaderboards",
    "",
    "exportCurrentImage",
    "applyCutClicked",
    "clearCutClicked",
    "cutLineScoreChanged",
    "value"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_TournamentLeaderboardDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x0a,    1 /* Public */,
       3,    0,   45,    2, 0x08,    2 /* Private */,
       4,    0,   46,    2, 0x08,    3 /* Private */,
       5,    0,   47,    2, 0x08,    4 /* Private */,
       6,    1,   48,    2, 0x08,    5 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,

       0        // eod
};

Q_CONSTINIT const QMetaObject TournamentLeaderboardDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_TournamentLeaderboardDialog.offsetsAndSizes,
    qt_meta_data_TournamentLeaderboardDialog,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_TournamentLeaderboardDialog_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TournamentLeaderboardDialog, std::true_type>,
        // method 'refreshLeaderboards'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'exportCurrentImage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'applyCutClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearCutClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cutLineScoreChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void TournamentLeaderboardDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TournamentLeaderboardDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->refreshLeaderboards(); break;
        case 1: _t->exportCurrentImage(); break;
        case 2: _t->applyCutClicked(); break;
        case 3: _t->clearCutClicked(); break;
        case 4: _t->cutLineScoreChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *TournamentLeaderboardDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TournamentLeaderboardDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TournamentLeaderboardDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int TournamentLeaderboardDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
