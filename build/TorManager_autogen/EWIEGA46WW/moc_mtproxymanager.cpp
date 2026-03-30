/****************************************************************************
** Meta object code from reading C++ file 'mtproxymanager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../mtproxymanager.h"
#include <QtNetwork/QSslError>
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mtproxymanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14MTProxyManagerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN14MTProxyManagerE = QtMocHelpers::stringData(
    "MTProxyManager",
    "clientConnected",
    "",
    "MTProxyClient",
    "client",
    "clientDisconnected",
    "connectionId",
    "clientActivity",
    "bytesRx",
    "bytesTx",
    "statsUpdated",
    "MTProxyStats",
    "stats",
    "logMessage",
    "message",
    "level",
    "proxyStarted",
    "proxyStopped",
    "proxyError",
    "error",
    "updateStats",
    "onProxyProcessOutput",
    "onProxyProcessError",
    "onProxyProcessFinished",
    "exitCode",
    "QProcess::ExitStatus",
    "status",
    "onGeoLookupFinished",
    "QNetworkReply*",
    "reply",
    "cleanupInactiveConnections"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN14MTProxyManagerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  104,    2, 0x06,    1 /* Public */,
       5,    1,  107,    2, 0x06,    3 /* Public */,
       7,    3,  110,    2, 0x06,    5 /* Public */,
      10,    1,  117,    2, 0x06,    9 /* Public */,
      13,    2,  120,    2, 0x06,   11 /* Public */,
      13,    1,  125,    2, 0x26,   14 /* Public | MethodCloned */,
      16,    0,  128,    2, 0x06,   16 /* Public */,
      17,    0,  129,    2, 0x06,   17 /* Public */,
      18,    1,  130,    2, 0x06,   18 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      20,    0,  133,    2, 0x0a,   20 /* Public */,
      21,    0,  134,    2, 0x08,   21 /* Private */,
      22,    0,  135,    2, 0x08,   22 /* Private */,
      23,    2,  136,    2, 0x08,   23 /* Private */,
      27,    1,  141,    2, 0x08,   26 /* Private */,
      30,    0,  144,    2, 0x08,   28 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong, QMetaType::LongLong,    6,    8,    9,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   14,   15,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   19,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 25,   24,   26,
    QMetaType::Void, 0x80000000 | 28,   29,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MTProxyManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN14MTProxyManagerE.offsetsAndSizes,
    qt_meta_data_ZN14MTProxyManagerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN14MTProxyManagerE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MTProxyManager, std::true_type>,
        // method 'clientConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MTProxyClient &, std::false_type>,
        // method 'clientDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clientActivity'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'statsUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MTProxyStats &, std::false_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'proxyStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'proxyStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'proxyError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateStats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProxyProcessOutput'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProxyProcessError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProxyProcessFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>,
        // method 'onGeoLookupFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        // method 'cleanupInactiveConnections'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MTProxyManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MTProxyManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->clientConnected((*reinterpret_cast< std::add_pointer_t<MTProxyClient>>(_a[1]))); break;
        case 1: _t->clientDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->clientActivity((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[3]))); break;
        case 3: _t->statsUpdated((*reinterpret_cast< std::add_pointer_t<MTProxyStats>>(_a[1]))); break;
        case 4: _t->logMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->logMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->proxyStarted(); break;
        case 7: _t->proxyStopped(); break;
        case 8: _t->proxyError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->updateStats(); break;
        case 10: _t->onProxyProcessOutput(); break;
        case 11: _t->onProxyProcessError(); break;
        case 12: _t->onProxyProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 13: _t->onGeoLookupFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 14: _t->cleanupInactiveConnections(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (MTProxyManager::*)(const MTProxyClient & );
            if (_q_method_type _q_method = &MTProxyManager::clientConnected; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (MTProxyManager::*)(const QString & );
            if (_q_method_type _q_method = &MTProxyManager::clientDisconnected; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (MTProxyManager::*)(const QString & , qint64 , qint64 );
            if (_q_method_type _q_method = &MTProxyManager::clientActivity; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (MTProxyManager::*)(const MTProxyStats & );
            if (_q_method_type _q_method = &MTProxyManager::statsUpdated; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (MTProxyManager::*)(const QString & , const QString & );
            if (_q_method_type _q_method = &MTProxyManager::logMessage; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (MTProxyManager::*)();
            if (_q_method_type _q_method = &MTProxyManager::proxyStarted; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (MTProxyManager::*)();
            if (_q_method_type _q_method = &MTProxyManager::proxyStopped; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (MTProxyManager::*)(const QString & );
            if (_q_method_type _q_method = &MTProxyManager::proxyError; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
    }
}

const QMetaObject *MTProxyManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MTProxyManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN14MTProxyManagerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MTProxyManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void MTProxyManager::clientConnected(const MTProxyClient & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MTProxyManager::clientDisconnected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void MTProxyManager::clientActivity(const QString & _t1, qint64 _t2, qint64 _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void MTProxyManager::statsUpdated(const MTProxyStats & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void MTProxyManager::logMessage(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 6
void MTProxyManager::proxyStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void MTProxyManager::proxyStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void MTProxyManager::proxyError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
namespace {
struct qt_meta_tag_ZN13MTProxyWidgetE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13MTProxyWidgetE = QtMocHelpers::stringData(
    "MTProxyWidget",
    "refreshData",
    "",
    "onClientConnected",
    "MTProxyClient",
    "client",
    "onClientDisconnected",
    "connectionId",
    "onStatsUpdated",
    "MTProxyStats",
    "stats",
    "onStartStopClicked",
    "onGenerateSecret",
    "onCopyLink",
    "onExportCSV",
    "onInstallMTProxy",
    "onClientTableContextMenu",
    "pos",
    "showClientDetails",
    "banClientIP"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13MTProxyWidgetE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   86,    2, 0x08,    1 /* Private */,
       3,    1,   87,    2, 0x08,    2 /* Private */,
       6,    1,   90,    2, 0x08,    4 /* Private */,
       8,    1,   93,    2, 0x08,    6 /* Private */,
      11,    0,   96,    2, 0x08,    8 /* Private */,
      12,    0,   97,    2, 0x08,    9 /* Private */,
      13,    0,   98,    2, 0x08,   10 /* Private */,
      14,    0,   99,    2, 0x08,   11 /* Private */,
      15,    0,  100,    2, 0x08,   12 /* Private */,
      16,    1,  101,    2, 0x08,   13 /* Private */,
      18,    0,  104,    2, 0x08,   15 /* Private */,
      19,    0,  105,    2, 0x08,   16 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   17,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MTProxyWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN13MTProxyWidgetE.offsetsAndSizes,
    qt_meta_data_ZN13MTProxyWidgetE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13MTProxyWidgetE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MTProxyWidget, std::true_type>,
        // method 'refreshData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onClientConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MTProxyClient &, std::false_type>,
        // method 'onClientDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onStatsUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MTProxyStats &, std::false_type>,
        // method 'onStartStopClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGenerateSecret'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCopyLink'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onExportCSV'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onInstallMTProxy'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onClientTableContextMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        // method 'showClientDetails'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'banClientIP'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MTProxyWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MTProxyWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->refreshData(); break;
        case 1: _t->onClientConnected((*reinterpret_cast< std::add_pointer_t<MTProxyClient>>(_a[1]))); break;
        case 2: _t->onClientDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onStatsUpdated((*reinterpret_cast< std::add_pointer_t<MTProxyStats>>(_a[1]))); break;
        case 4: _t->onStartStopClicked(); break;
        case 5: _t->onGenerateSecret(); break;
        case 6: _t->onCopyLink(); break;
        case 7: _t->onExportCSV(); break;
        case 8: _t->onInstallMTProxy(); break;
        case 9: _t->onClientTableContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 10: _t->showClientDetails(); break;
        case 11: _t->banClientIP(); break;
        default: ;
        }
    }
}

const QMetaObject *MTProxyWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MTProxyWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13MTProxyWidgetE.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MTProxyWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    return _id;
}
QT_WARNING_POP
