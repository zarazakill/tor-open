/****************************************************************************
** Meta object code from reading C++ file 'mtproxymanager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../mtproxymanager.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mtproxymanager.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_MTProxyManager_t {
    uint offsetsAndSizes[66];
    char stringdata0[15];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[14];
    char stringdata4[7];
    char stringdata5[19];
    char stringdata6[13];
    char stringdata7[15];
    char stringdata8[8];
    char stringdata9[8];
    char stringdata10[13];
    char stringdata11[13];
    char stringdata12[6];
    char stringdata13[11];
    char stringdata14[8];
    char stringdata15[6];
    char stringdata16[13];
    char stringdata17[13];
    char stringdata18[11];
    char stringdata19[6];
    char stringdata20[12];
    char stringdata21[19];
    char stringdata22[3];
    char stringdata23[21];
    char stringdata24[20];
    char stringdata25[23];
    char stringdata26[9];
    char stringdata27[21];
    char stringdata28[7];
    char stringdata29[20];
    char stringdata30[15];
    char stringdata31[6];
    char stringdata32[27];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MTProxyManager_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MTProxyManager_t qt_meta_stringdata_MTProxyManager = {
    {
        QT_MOC_LITERAL(0, 14),  // "MTProxyManager"
        QT_MOC_LITERAL(15, 15),  // "clientConnected"
        QT_MOC_LITERAL(31, 0),  // ""
        QT_MOC_LITERAL(32, 13),  // "MTProxyClient"
        QT_MOC_LITERAL(46, 6),  // "client"
        QT_MOC_LITERAL(53, 18),  // "clientDisconnected"
        QT_MOC_LITERAL(72, 12),  // "connectionId"
        QT_MOC_LITERAL(85, 14),  // "clientActivity"
        QT_MOC_LITERAL(100, 7),  // "bytesRx"
        QT_MOC_LITERAL(108, 7),  // "bytesTx"
        QT_MOC_LITERAL(116, 12),  // "statsUpdated"
        QT_MOC_LITERAL(129, 12),  // "MTProxyStats"
        QT_MOC_LITERAL(142, 5),  // "stats"
        QT_MOC_LITERAL(148, 10),  // "logMessage"
        QT_MOC_LITERAL(159, 7),  // "message"
        QT_MOC_LITERAL(167, 5),  // "level"
        QT_MOC_LITERAL(173, 12),  // "proxyStarted"
        QT_MOC_LITERAL(186, 12),  // "proxyStopped"
        QT_MOC_LITERAL(199, 10),  // "proxyError"
        QT_MOC_LITERAL(210, 5),  // "error"
        QT_MOC_LITERAL(216, 11),  // "updateStats"
        QT_MOC_LITERAL(228, 18),  // "refreshGeoLocation"
        QT_MOC_LITERAL(247, 2),  // "ip"
        QT_MOC_LITERAL(250, 20),  // "onProxyProcessOutput"
        QT_MOC_LITERAL(271, 19),  // "onProxyProcessError"
        QT_MOC_LITERAL(291, 22),  // "onProxyProcessFinished"
        QT_MOC_LITERAL(314, 8),  // "exitCode"
        QT_MOC_LITERAL(323, 20),  // "QProcess::ExitStatus"
        QT_MOC_LITERAL(344, 6),  // "status"
        QT_MOC_LITERAL(351, 19),  // "onGeoLookupFinished"
        QT_MOC_LITERAL(371, 14),  // "QNetworkReply*"
        QT_MOC_LITERAL(386, 5),  // "reply"
        QT_MOC_LITERAL(392, 26)   // "cleanupInactiveConnections"
    },
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
    "refreshGeoLocation",
    "ip",
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
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MTProxyManager[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  110,    2, 0x06,    1 /* Public */,
       5,    1,  113,    2, 0x06,    3 /* Public */,
       7,    3,  116,    2, 0x06,    5 /* Public */,
      10,    1,  123,    2, 0x06,    9 /* Public */,
      13,    2,  126,    2, 0x06,   11 /* Public */,
      13,    1,  131,    2, 0x26,   14 /* Public | MethodCloned */,
      16,    0,  134,    2, 0x06,   16 /* Public */,
      17,    0,  135,    2, 0x06,   17 /* Public */,
      18,    1,  136,    2, 0x06,   18 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      20,    0,  139,    2, 0x0a,   20 /* Public */,
      21,    1,  140,    2, 0x0a,   21 /* Public */,
      23,    0,  143,    2, 0x08,   23 /* Private */,
      24,    0,  144,    2, 0x08,   24 /* Private */,
      25,    2,  145,    2, 0x08,   25 /* Private */,
      29,    1,  150,    2, 0x08,   28 /* Private */,
      32,    0,  153,    2, 0x08,   30 /* Private */,

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
    QMetaType::Void, QMetaType::QString,   22,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 27,   26,   28,
    QMetaType::Void, 0x80000000 | 30,   31,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MTProxyManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_MTProxyManager.offsetsAndSizes,
    qt_meta_data_MTProxyManager,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MTProxyManager_t,
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
        // method 'refreshGeoLocation'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
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
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MTProxyManager *>(_o);
        (void)_t;
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
        case 10: _t->refreshGeoLocation((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->onProxyProcessOutput(); break;
        case 12: _t->onProxyProcessError(); break;
        case 13: _t->onProxyProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 14: _t->onGeoLookupFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 15: _t->cleanupInactiveConnections(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MTProxyManager::*)(const MTProxyClient & );
            if (_t _q_method = &MTProxyManager::clientConnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MTProxyManager::*)(const QString & );
            if (_t _q_method = &MTProxyManager::clientDisconnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MTProxyManager::*)(const QString & , qint64 , qint64 );
            if (_t _q_method = &MTProxyManager::clientActivity; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MTProxyManager::*)(const MTProxyStats & );
            if (_t _q_method = &MTProxyManager::statsUpdated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MTProxyManager::*)(const QString & , const QString & );
            if (_t _q_method = &MTProxyManager::logMessage; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MTProxyManager::*)();
            if (_t _q_method = &MTProxyManager::proxyStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (MTProxyManager::*)();
            if (_t _q_method = &MTProxyManager::proxyStopped; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (MTProxyManager::*)(const QString & );
            if (_t _q_method = &MTProxyManager::proxyError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
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
    if (!strcmp(_clname, qt_meta_stringdata_MTProxyManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MTProxyManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
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
struct qt_meta_stringdata_MTProxyWidget_t {
    uint offsetsAndSizes[38];
    char stringdata0[14];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[18];
    char stringdata4[14];
    char stringdata5[7];
    char stringdata6[21];
    char stringdata7[13];
    char stringdata8[15];
    char stringdata9[13];
    char stringdata10[6];
    char stringdata11[19];
    char stringdata12[17];
    char stringdata13[11];
    char stringdata14[12];
    char stringdata15[25];
    char stringdata16[4];
    char stringdata17[18];
    char stringdata18[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MTProxyWidget_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MTProxyWidget_t qt_meta_stringdata_MTProxyWidget = {
    {
        QT_MOC_LITERAL(0, 13),  // "MTProxyWidget"
        QT_MOC_LITERAL(14, 11),  // "refreshData"
        QT_MOC_LITERAL(26, 0),  // ""
        QT_MOC_LITERAL(27, 17),  // "onClientConnected"
        QT_MOC_LITERAL(45, 13),  // "MTProxyClient"
        QT_MOC_LITERAL(59, 6),  // "client"
        QT_MOC_LITERAL(66, 20),  // "onClientDisconnected"
        QT_MOC_LITERAL(87, 12),  // "connectionId"
        QT_MOC_LITERAL(100, 14),  // "onStatsUpdated"
        QT_MOC_LITERAL(115, 12),  // "MTProxyStats"
        QT_MOC_LITERAL(128, 5),  // "stats"
        QT_MOC_LITERAL(134, 18),  // "onStartStopClicked"
        QT_MOC_LITERAL(153, 16),  // "onGenerateSecret"
        QT_MOC_LITERAL(170, 10),  // "onCopyLink"
        QT_MOC_LITERAL(181, 11),  // "onExportCSV"
        QT_MOC_LITERAL(193, 24),  // "onClientTableContextMenu"
        QT_MOC_LITERAL(218, 3),  // "pos"
        QT_MOC_LITERAL(222, 17),  // "showClientDetails"
        QT_MOC_LITERAL(240, 11)   // "banClientIP"
    },
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
    "onClientTableContextMenu",
    "pos",
    "showClientDetails",
    "banClientIP"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MTProxyWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   80,    2, 0x08,    1 /* Private */,
       3,    1,   81,    2, 0x08,    2 /* Private */,
       6,    1,   84,    2, 0x08,    4 /* Private */,
       8,    1,   87,    2, 0x08,    6 /* Private */,
      11,    0,   90,    2, 0x08,    8 /* Private */,
      12,    0,   91,    2, 0x08,    9 /* Private */,
      13,    0,   92,    2, 0x08,   10 /* Private */,
      14,    0,   93,    2, 0x08,   11 /* Private */,
      15,    1,   94,    2, 0x08,   12 /* Private */,
      17,    0,   97,    2, 0x08,   14 /* Private */,
      18,    0,   98,    2, 0x08,   15 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   16,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MTProxyWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_MTProxyWidget.offsetsAndSizes,
    qt_meta_data_MTProxyWidget,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MTProxyWidget_t,
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
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MTProxyWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->refreshData(); break;
        case 1: _t->onClientConnected((*reinterpret_cast< std::add_pointer_t<MTProxyClient>>(_a[1]))); break;
        case 2: _t->onClientDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onStatsUpdated((*reinterpret_cast< std::add_pointer_t<MTProxyStats>>(_a[1]))); break;
        case 4: _t->onStartStopClicked(); break;
        case 5: _t->onGenerateSecret(); break;
        case 6: _t->onCopyLink(); break;
        case 7: _t->onExportCSV(); break;
        case 8: _t->onClientTableContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 9: _t->showClientDetails(); break;
        case 10: _t->banClientIP(); break;
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
    if (!strcmp(_clname, qt_meta_stringdata_MTProxyWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MTProxyWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
