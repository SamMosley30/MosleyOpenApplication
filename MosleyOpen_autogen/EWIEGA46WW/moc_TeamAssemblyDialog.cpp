/****************************************************************************
** Meta object code from reading C++ file 'TeamAssemblyDialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../TeamAssemblyDialog.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TeamAssemblyDialog.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_TeamAssemblyDialog_t {
    uint offsetsAndSizes[26];
    char stringdata0[19];
    char stringdata1[18];
    char stringdata2[1];
    char stringdata3[16];
    char stringdata4[10];
    char stringdata5[20];
    char stringdata6[11];
    char stringdata7[7];
    char stringdata8[18];
    char stringdata9[11];
    char stringdata10[11];
    char stringdata11[8];
    char stringdata12[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_TeamAssemblyDialog_t::offsetsAndSizes) + ofs), len
Q_CONSTINIT static const qt_meta_stringdata_TeamAssemblyDialog_t qt_meta_stringdata_TeamAssemblyDialog = {
    {
        QT_MOC_LITERAL(0, 18),  // "TeamAssemblyDialog"
        QT_MOC_LITERAL(19, 17),  // "loadActivePlayers"
        QT_MOC_LITERAL(37, 0),  // ""
        QT_MOC_LITERAL(38, 15),  // "autoAssignTeams"
        QT_MOC_LITERAL(54, 9),  // "saveTeams"
        QT_MOC_LITERAL(64, 19),  // "handlePlayerDropped"
        QT_MOC_LITERAL(84, 10),  // "PlayerInfo"
        QT_MOC_LITERAL(95, 6),  // "player"
        QT_MOC_LITERAL(102, 17),  // "PlayerListWidget*"
        QT_MOC_LITERAL(120, 10),  // "sourceList"
        QT_MOC_LITERAL(131, 10),  // "targetList"
        QT_MOC_LITERAL(142, 7),  // "addTeam"
        QT_MOC_LITERAL(150, 10)   // "removeTeam"
    },
    "TeamAssemblyDialog",
    "loadActivePlayers",
    "",
    "autoAssignTeams",
    "saveTeams",
    "handlePlayerDropped",
    "PlayerInfo",
    "player",
    "PlayerListWidget*",
    "sourceList",
    "targetList",
    "addTeam",
    "removeTeam"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_TeamAssemblyDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x08,    1 /* Private */,
       3,    0,   51,    2, 0x08,    2 /* Private */,
       4,    0,   52,    2, 0x08,    3 /* Private */,
       5,    3,   53,    2, 0x08,    4 /* Private */,
      11,    0,   60,    2, 0x08,    8 /* Private */,
      12,    0,   61,    2, 0x08,    9 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 8, 0x80000000 | 8,    7,    9,   10,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject TeamAssemblyDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_TeamAssemblyDialog.offsetsAndSizes,
    qt_meta_data_TeamAssemblyDialog,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_TeamAssemblyDialog_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TeamAssemblyDialog, std::true_type>,
        // method 'loadActivePlayers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'autoAssignTeams'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveTeams'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handlePlayerDropped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const PlayerInfo &, std::false_type>,
        QtPrivate::TypeAndForceComplete<PlayerListWidget *, std::false_type>,
        QtPrivate::TypeAndForceComplete<PlayerListWidget *, std::false_type>,
        // method 'addTeam'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'removeTeam'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void TeamAssemblyDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TeamAssemblyDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->loadActivePlayers(); break;
        case 1: _t->autoAssignTeams(); break;
        case 2: _t->saveTeams(); break;
        case 3: _t->handlePlayerDropped((*reinterpret_cast< std::add_pointer_t<PlayerInfo>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<PlayerListWidget*>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<PlayerListWidget*>>(_a[3]))); break;
        case 4: _t->addTeam(); break;
        case 5: _t->removeTeam(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 2:
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< PlayerListWidget* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *TeamAssemblyDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TeamAssemblyDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TeamAssemblyDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int TeamAssemblyDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
