/****************************************************************************
** Meta object code from reading C++ file 'dns_monitor.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../dns_monitor.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dns_monitor.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_DnsMonitor_t {
    uint offsetsAndSizes[26];
    char stringdata0[11];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[10];
    char stringdata4[7];
    char stringdata5[11];
    char stringdata6[4];
    char stringdata7[5];
    char stringdata8[16];
    char stringdata9[15];
    char stringdata10[17];
    char stringdata11[14];
    char stringdata12[14];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_DnsMonitor_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_DnsMonitor_t qt_meta_stringdata_DnsMonitor = {
    {
        QT_MOC_LITERAL(0, 10),  // "DnsMonitor"
        QT_MOC_LITERAL(11, 11),  // "urlAccessed"
        QT_MOC_LITERAL(23, 0),  // ""
        QT_MOC_LITERAL(24, 9),  // "UrlAccess"
        QT_MOC_LITERAL(34, 6),  // "access"
        QT_MOC_LITERAL(41, 10),  // "logMessage"
        QT_MOC_LITERAL(52, 3),  // "msg"
        QT_MOC_LITERAL(56, 4),  // "type"
        QT_MOC_LITERAL(61, 15),  // "onTcpdumpOutput"
        QT_MOC_LITERAL(77, 14),  // "onTorLogOutput"
        QT_MOC_LITERAL(92, 16),  // "onTorLogFinished"
        QT_MOC_LITERAL(109, 13),  // "onControlData"
        QT_MOC_LITERAL(123, 13)   // "pollConntrack"
    },
    "DnsMonitor",
    "urlAccessed",
    "",
    "UrlAccess",
    "access",
    "logMessage",
    "msg",
    "type",
    "onTcpdumpOutput",
    "onTorLogOutput",
    "onTorLogFinished",
    "onControlData",
    "pollConntrack"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_DnsMonitor[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   56,    2, 0x06,    1 /* Public */,
       5,    2,   59,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,   64,    2, 0x08,    6 /* Private */,
       9,    0,   65,    2, 0x08,    7 /* Private */,
      10,    0,   66,    2, 0x08,    8 /* Private */,
      11,    0,   67,    2, 0x08,    9 /* Private */,
      12,    0,   68,    2, 0x08,   10 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    6,    7,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject DnsMonitor::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_DnsMonitor.offsetsAndSizes,
    qt_meta_data_DnsMonitor,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_DnsMonitor_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DnsMonitor, std::true_type>,
        // method 'urlAccessed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const UrlAccess &, std::false_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onTcpdumpOutput'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTorLogOutput'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTorLogFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onControlData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pollConntrack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void DnsMonitor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DnsMonitor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->urlAccessed((*reinterpret_cast< std::add_pointer_t<UrlAccess>>(_a[1]))); break;
        case 1: _t->logMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->onTcpdumpOutput(); break;
        case 3: _t->onTorLogOutput(); break;
        case 4: _t->onTorLogFinished(); break;
        case 5: _t->onControlData(); break;
        case 6: _t->pollConntrack(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DnsMonitor::*)(const UrlAccess & );
            if (_t _q_method = &DnsMonitor::urlAccessed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DnsMonitor::*)(const QString & , const QString & );
            if (_t _q_method = &DnsMonitor::logMessage; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *DnsMonitor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DnsMonitor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DnsMonitor.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DnsMonitor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void DnsMonitor::urlAccessed(const UrlAccess & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DnsMonitor::logMessage(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
