/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CertificateGenerator_t {
    uint offsetsAndSizes[36];
    char stringdata0[21];
    char stringdata1[11];
    char stringdata2[1];
    char stringdata3[8];
    char stringdata4[5];
    char stringdata5[9];
    char stringdata6[8];
    char stringdata7[9];
    char stringdata8[8];
    char stringdata9[18];
    char stringdata10[9];
    char stringdata11[21];
    char stringdata12[11];
    char stringdata13[15];
    char stringdata14[23];
    char stringdata15[6];
    char stringdata16[16];
    char stringdata17[21];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CertificateGenerator_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CertificateGenerator_t qt_meta_stringdata_CertificateGenerator = {
    {
        QT_MOC_LITERAL(0, 20),  // "CertificateGenerator"
        QT_MOC_LITERAL(21, 10),  // "logMessage"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 7),  // "message"
        QT_MOC_LITERAL(41, 4),  // "type"
        QT_MOC_LITERAL(46, 8),  // "finished"
        QT_MOC_LITERAL(55, 7),  // "success"
        QT_MOC_LITERAL(63, 8),  // "progress"
        QT_MOC_LITERAL(72, 7),  // "percent"
        QT_MOC_LITERAL(80, 17),  // "onProcessFinished"
        QT_MOC_LITERAL(98, 8),  // "exitCode"
        QT_MOC_LITERAL(107, 20),  // "QProcess::ExitStatus"
        QT_MOC_LITERAL(128, 10),  // "exitStatus"
        QT_MOC_LITERAL(139, 14),  // "onProcessError"
        QT_MOC_LITERAL(154, 22),  // "QProcess::ProcessError"
        QT_MOC_LITERAL(177, 5),  // "error"
        QT_MOC_LITERAL(183, 15),  // "onProcessOutput"
        QT_MOC_LITERAL(199, 20)   // "onProcessErrorOutput"
    },
    "CertificateGenerator",
    "logMessage",
    "",
    "message",
    "type",
    "finished",
    "success",
    "progress",
    "percent",
    "onProcessFinished",
    "exitCode",
    "QProcess::ExitStatus",
    "exitStatus",
    "onProcessError",
    "QProcess::ProcessError",
    "error",
    "onProcessOutput",
    "onProcessErrorOutput"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CertificateGenerator[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   56,    2, 0x06,    1 /* Public */,
       5,    1,   61,    2, 0x06,    4 /* Public */,
       7,    1,   64,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       9,    2,   67,    2, 0x08,    8 /* Private */,
      13,    1,   72,    2, 0x08,   11 /* Private */,
      16,    0,   75,    2, 0x08,   13 /* Private */,
      17,    0,   76,    2, 0x08,   14 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void, QMetaType::Int,    8,

 // slots: parameters
    QMetaType::Void, QMetaType::Int, 0x80000000 | 11,   10,   12,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject CertificateGenerator::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CertificateGenerator.offsetsAndSizes,
    qt_meta_data_CertificateGenerator,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CertificateGenerator_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CertificateGenerator, std::true_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'finished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'progress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onProcessFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>,
        // method 'onProcessError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>,
        // method 'onProcessOutput'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProcessErrorOutput'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void CertificateGenerator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CertificateGenerator *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->logMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->finished((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->progress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->onProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 4: _t->onProcessError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 5: _t->onProcessOutput(); break;
        case 6: _t->onProcessErrorOutput(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CertificateGenerator::*)(const QString & , const QString & );
            if (_t _q_method = &CertificateGenerator::logMessage; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CertificateGenerator::*)(bool );
            if (_t _q_method = &CertificateGenerator::finished; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CertificateGenerator::*)(int );
            if (_t _q_method = &CertificateGenerator::progress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *CertificateGenerator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CertificateGenerator::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CertificateGenerator.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CertificateGenerator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void CertificateGenerator::logMessage(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CertificateGenerator::finished(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CertificateGenerator::progress(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
namespace {
struct qt_meta_stringdata_MainWindow_t {
    uint offsetsAndSizes[260];
    char stringdata0[11];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[15];
    char stringdata4[16];
    char stringdata5[8];
    char stringdata6[16];
    char stringdata7[27];
    char stringdata8[9];
    char stringdata9[22];
    char stringdata10[9];
    char stringdata11[19];
    char stringdata12[18];
    char stringdata13[27];
    char stringdata14[3];
    char stringdata15[13];
    char stringdata16[7];
    char stringdata17[13];
    char stringdata18[12];
    char stringdata19[8];
    char stringdata20[11];
    char stringdata21[13];
    char stringdata22[14];
    char stringdata23[9];
    char stringdata24[21];
    char stringdata25[11];
    char stringdata26[11];
    char stringdata27[23];
    char stringdata28[6];
    char stringdata29[15];
    char stringdata30[15];
    char stringdata31[20];
    char stringdata32[5];
    char stringdata33[15];
    char stringdata34[8];
    char stringdata35[25];
    char stringdata36[25];
    char stringdata37[21];
    char stringdata38[18];
    char stringdata39[18];
    char stringdata40[16];
    char stringdata41[17];
    char stringdata42[14];
    char stringdata43[18];
    char stringdata44[21];
    char stringdata45[26];
    char stringdata46[25];
    char stringdata47[8];
    char stringdata48[18];
    char stringdata49[21];
    char stringdata50[26];
    char stringdata51[26];
    char stringdata52[11];
    char stringdata53[12];
    char stringdata54[18];
    char stringdata55[13];
    char stringdata56[13];
    char stringdata57[10];
    char stringdata58[16];
    char stringdata59[34];
    char stringdata60[7];
    char stringdata61[14];
    char stringdata62[19];
    char stringdata63[14];
    char stringdata64[8];
    char stringdata65[5];
    char stringdata66[10];
    char stringdata67[10];
    char stringdata68[13];
    char stringdata69[22];
    char stringdata70[21];
    char stringdata71[21];
    char stringdata72[7];
    char stringdata73[19];
    char stringdata74[17];
    char stringdata75[18];
    char stringdata76[19];
    char stringdata77[7];
    char stringdata78[19];
    char stringdata79[17];
    char stringdata80[21];
    char stringdata81[19];
    char stringdata82[21];
    char stringdata83[14];
    char stringdata84[19];
    char stringdata85[18];
    char stringdata86[17];
    char stringdata87[19];
    char stringdata88[21];
    char stringdata89[25];
    char stringdata90[11];
    char stringdata91[4];
    char stringdata92[25];
    char stringdata93[21];
    char stringdata94[18];
    char stringdata95[10];
    char stringdata96[16];
    char stringdata97[5];
    char stringdata98[17];
    char stringdata99[16];
    char stringdata100[25];
    char stringdata101[4];
    char stringdata102[18];
    char stringdata103[26];
    char stringdata104[20];
    char stringdata105[25];
    char stringdata106[19];
    char stringdata107[19];
    char stringdata108[20];
    char stringdata109[20];
    char stringdata110[24];
    char stringdata111[25];
    char stringdata112[24];
    char stringdata113[18];
    char stringdata114[18];
    char stringdata115[19];
    char stringdata116[19];
    char stringdata117[20];
    char stringdata118[20];
    char stringdata119[13];
    char stringdata120[10];
    char stringdata121[7];
    char stringdata122[15];
    char stringdata123[22];
    char stringdata124[17];
    char stringdata125[17];
    char stringdata126[17];
    char stringdata127[13];
    char stringdata128[4];
    char stringdata129[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "MainWindow"
        QT_MOC_LITERAL(11, 13),  // "serverStarted"
        QT_MOC_LITERAL(25, 0),  // ""
        QT_MOC_LITERAL(26, 14),  // "clientsUpdated"
        QT_MOC_LITERAL(41, 15),  // "torStateChanged"
        QT_MOC_LITERAL(57, 7),  // "running"
        QT_MOC_LITERAL(65, 15),  // "registryUpdated"
        QT_MOC_LITERAL(81, 26),  // "QMap<QString,ClientRecord>"
        QT_MOC_LITERAL(108, 8),  // "registry"
        QT_MOC_LITERAL(117, 21),  // "clientRegistryChanged"
        QT_MOC_LITERAL(139, 8),  // "startTor"
        QT_MOC_LITERAL(148, 18),  // "startOpenVPNServer"
        QT_MOC_LITERAL(167, 17),  // "getClientRegistry"
        QT_MOC_LITERAL(185, 26),  // "updateRegistryFromTelegram"
        QT_MOC_LITERAL(212, 2),  // "cn"
        QT_MOC_LITERAL(215, 12),  // "ClientRecord"
        QT_MOC_LITERAL(228, 6),  // "record"
        QT_MOC_LITERAL(235, 12),  // "startMTProxy"
        QT_MOC_LITERAL(248, 11),  // "stopMTProxy"
        QT_MOC_LITERAL(260, 7),  // "stopTor"
        QT_MOC_LITERAL(268, 10),  // "restartTor"
        QT_MOC_LITERAL(279, 12),  // "onTorStarted"
        QT_MOC_LITERAL(292, 13),  // "onTorFinished"
        QT_MOC_LITERAL(306, 8),  // "exitCode"
        QT_MOC_LITERAL(315, 20),  // "QProcess::ExitStatus"
        QT_MOC_LITERAL(336, 10),  // "exitStatus"
        QT_MOC_LITERAL(347, 10),  // "onTorError"
        QT_MOC_LITERAL(358, 22),  // "QProcess::ProcessError"
        QT_MOC_LITERAL(381, 5),  // "error"
        QT_MOC_LITERAL(387, 14),  // "onTorReadyRead"
        QT_MOC_LITERAL(402, 14),  // "checkTorStatus"
        QT_MOC_LITERAL(417, 19),  // "parseTorCircuitLine"
        QT_MOC_LITERAL(437, 4),  // "line"
        QT_MOC_LITERAL(442, 14),  // "sendTorCommand"
        QT_MOC_LITERAL(457, 7),  // "command"
        QT_MOC_LITERAL(465, 24),  // "onControlSocketConnected"
        QT_MOC_LITERAL(490, 24),  // "onControlSocketReadyRead"
        QT_MOC_LITERAL(515, 20),  // "onControlSocketError"
        QT_MOC_LITERAL(536, 17),  // "requestNewCircuit"
        QT_MOC_LITERAL(554, 17),  // "stopOpenVPNServer"
        QT_MOC_LITERAL(572, 15),  // "onServerStarted"
        QT_MOC_LITERAL(588, 16),  // "onServerFinished"
        QT_MOC_LITERAL(605, 13),  // "onServerError"
        QT_MOC_LITERAL(619, 17),  // "onServerReadyRead"
        QT_MOC_LITERAL(637, 20),  // "generateCertificates"
        QT_MOC_LITERAL(658, 25),  // "generateCertificatesAsync"
        QT_MOC_LITERAL(684, 24),  // "onCertGenerationFinished"
        QT_MOC_LITERAL(709, 7),  // "success"
        QT_MOC_LITERAL(717, 17),  // "checkCertificates"
        QT_MOC_LITERAL(735, 20),  // "generateClientConfig"
        QT_MOC_LITERAL(756, 25),  // "generateTestAndroidConfig"
        QT_MOC_LITERAL(782, 25),  // "generateClientCertificate"
        QT_MOC_LITERAL(808, 10),  // "clientName"
        QT_MOC_LITERAL(819, 11),  // "checkIPLeak"
        QT_MOC_LITERAL(831, 17),  // "onIPCheckFinished"
        QT_MOC_LITERAL(849, 12),  // "updateStatus"
        QT_MOC_LITERAL(862, 12),  // "showSettings"
        QT_MOC_LITERAL(875, 9),  // "showAbout"
        QT_MOC_LITERAL(885, 15),  // "onTrayActivated"
        QT_MOC_LITERAL(901, 33),  // "QSystemTrayIcon::ActivationRe..."
        QT_MOC_LITERAL(935, 6),  // "reason"
        QT_MOC_LITERAL(942, 13),  // "applySettings"
        QT_MOC_LITERAL(956, 18),  // "updateTrafficStats"
        QT_MOC_LITERAL(975, 13),  // "addLogMessage"
        QT_MOC_LITERAL(989, 7),  // "message"
        QT_MOC_LITERAL(997, 4),  // "type"
        QT_MOC_LITERAL(1002, 9),  // "flushLogs"
        QT_MOC_LITERAL(1012, 9),  // "addBridge"
        QT_MOC_LITERAL(1022, 12),  // "removeBridge"
        QT_MOC_LITERAL(1035, 21),  // "importBridgesFromText"
        QT_MOC_LITERAL(1057, 20),  // "validateBridgeFormat"
        QT_MOC_LITERAL(1078, 20),  // "testBridgeConnection"
        QT_MOC_LITERAL(1099, 6),  // "bridge"
        QT_MOC_LITERAL(1106, 18),  // "updateBridgeConfig"
        QT_MOC_LITERAL(1125, 16),  // "enableKillSwitch"
        QT_MOC_LITERAL(1142, 17),  // "disableKillSwitch"
        QT_MOC_LITERAL(1160, 18),  // "setupFirewallRules"
        QT_MOC_LITERAL(1179, 6),  // "enable"
        QT_MOC_LITERAL(1186, 18),  // "diagnoseConnection"
        QT_MOC_LITERAL(1205, 16),  // "testServerConfig"
        QT_MOC_LITERAL(1222, 20),  // "getExternalInterface"
        QT_MOC_LITERAL(1243, 18),  // "setupIPTablesRules"
        QT_MOC_LITERAL(1262, 20),  // "applyRoutingManually"
        QT_MOC_LITERAL(1283, 13),  // "verifyRouting"
        QT_MOC_LITERAL(1297, 18),  // "enableIPForwarding"
        QT_MOC_LITERAL(1316, 17),  // "checkIPForwarding"
        QT_MOC_LITERAL(1334, 16),  // "createClientsTab"
        QT_MOC_LITERAL(1351, 18),  // "updateClientsTable"
        QT_MOC_LITERAL(1370, 20),  // "updateClientsTableUI"
        QT_MOC_LITERAL(1391, 24),  // "QMap<QString,ClientInfo>"
        QT_MOC_LITERAL(1416, 10),  // "newClients"
        QT_MOC_LITERAL(1427, 3),  // "now"
        QT_MOC_LITERAL(1431, 24),  // "disconnectSelectedClient"
        QT_MOC_LITERAL(1456, 20),  // "disconnectAllClients"
        QT_MOC_LITERAL(1477, 17),  // "showClientDetails"
        QT_MOC_LITERAL(1495, 9),  // "banClient"
        QT_MOC_LITERAL(1505, 15),  // "applySpeedLimit"
        QT_MOC_LITERAL(1521, 4),  // "kbps"
        QT_MOC_LITERAL(1526, 16),  // "exportClientsLog"
        QT_MOC_LITERAL(1543, 15),  // "clearClientsLog"
        QT_MOC_LITERAL(1559, 24),  // "onClientTableContextMenu"
        QT_MOC_LITERAL(1584, 3),  // "pos"
        QT_MOC_LITERAL(1588, 17),  // "refreshClientsNow"
        QT_MOC_LITERAL(1606, 25),  // "generateNamedClientConfig"
        QT_MOC_LITERAL(1632, 19),  // "showClientAnalytics"
        QT_MOC_LITERAL(1652, 24),  // "showClientSessionHistory"
        QT_MOC_LITERAL(1677, 18),  // "loadClientRegistry"
        QT_MOC_LITERAL(1696, 18),  // "saveClientRegistry"
        QT_MOC_LITERAL(1715, 19),  // "updateRegistryTable"
        QT_MOC_LITERAL(1735, 19),  // "applyRegistryFilter"
        QT_MOC_LITERAL(1755, 23),  // "updateRealtimeDurations"
        QT_MOC_LITERAL(1779, 24),  // "checkMultipleConnections"
        QT_MOC_LITERAL(1804, 23),  // "deleteClientPermanently"
        QT_MOC_LITERAL(1828, 17),  // "updateClientStats"
        QT_MOC_LITERAL(1846, 17),  // "createTelegramTab"
        QT_MOC_LITERAL(1864, 18),  // "onBotClientCreated"
        QT_MOC_LITERAL(1883, 18),  // "onBotClientRevoked"
        QT_MOC_LITERAL(1902, 19),  // "syncBotWithSettings"
        QT_MOC_LITERAL(1922, 19),  // "createUrlHistoryTab"
        QT_MOC_LITERAL(1942, 12),  // "addUrlAccess"
        QT_MOC_LITERAL(1955, 9),  // "UrlAccess"
        QT_MOC_LITERAL(1965, 6),  // "access"
        QT_MOC_LITERAL(1972, 14),  // "updateUrlTable"
        QT_MOC_LITERAL(1987, 21),  // "diagnoseBotConnection"
        QT_MOC_LITERAL(2009, 16),  // "createMTProxyTab"
        QT_MOC_LITERAL(2026, 16),  // "onMTProxyStarted"
        QT_MOC_LITERAL(2043, 16),  // "onMTProxyStopped"
        QT_MOC_LITERAL(2060, 12),  // "onMTProxyLog"
        QT_MOC_LITERAL(2073, 3),  // "msg"
        QT_MOC_LITERAL(2077, 5)   // "level"
    },
    "MainWindow",
    "serverStarted",
    "",
    "clientsUpdated",
    "torStateChanged",
    "running",
    "registryUpdated",
    "QMap<QString,ClientRecord>",
    "registry",
    "clientRegistryChanged",
    "startTor",
    "startOpenVPNServer",
    "getClientRegistry",
    "updateRegistryFromTelegram",
    "cn",
    "ClientRecord",
    "record",
    "startMTProxy",
    "stopMTProxy",
    "stopTor",
    "restartTor",
    "onTorStarted",
    "onTorFinished",
    "exitCode",
    "QProcess::ExitStatus",
    "exitStatus",
    "onTorError",
    "QProcess::ProcessError",
    "error",
    "onTorReadyRead",
    "checkTorStatus",
    "parseTorCircuitLine",
    "line",
    "sendTorCommand",
    "command",
    "onControlSocketConnected",
    "onControlSocketReadyRead",
    "onControlSocketError",
    "requestNewCircuit",
    "stopOpenVPNServer",
    "onServerStarted",
    "onServerFinished",
    "onServerError",
    "onServerReadyRead",
    "generateCertificates",
    "generateCertificatesAsync",
    "onCertGenerationFinished",
    "success",
    "checkCertificates",
    "generateClientConfig",
    "generateTestAndroidConfig",
    "generateClientCertificate",
    "clientName",
    "checkIPLeak",
    "onIPCheckFinished",
    "updateStatus",
    "showSettings",
    "showAbout",
    "onTrayActivated",
    "QSystemTrayIcon::ActivationReason",
    "reason",
    "applySettings",
    "updateTrafficStats",
    "addLogMessage",
    "message",
    "type",
    "flushLogs",
    "addBridge",
    "removeBridge",
    "importBridgesFromText",
    "validateBridgeFormat",
    "testBridgeConnection",
    "bridge",
    "updateBridgeConfig",
    "enableKillSwitch",
    "disableKillSwitch",
    "setupFirewallRules",
    "enable",
    "diagnoseConnection",
    "testServerConfig",
    "getExternalInterface",
    "setupIPTablesRules",
    "applyRoutingManually",
    "verifyRouting",
    "enableIPForwarding",
    "checkIPForwarding",
    "createClientsTab",
    "updateClientsTable",
    "updateClientsTableUI",
    "QMap<QString,ClientInfo>",
    "newClients",
    "now",
    "disconnectSelectedClient",
    "disconnectAllClients",
    "showClientDetails",
    "banClient",
    "applySpeedLimit",
    "kbps",
    "exportClientsLog",
    "clearClientsLog",
    "onClientTableContextMenu",
    "pos",
    "refreshClientsNow",
    "generateNamedClientConfig",
    "showClientAnalytics",
    "showClientSessionHistory",
    "loadClientRegistry",
    "saveClientRegistry",
    "updateRegistryTable",
    "applyRegistryFilter",
    "updateRealtimeDurations",
    "checkMultipleConnections",
    "deleteClientPermanently",
    "updateClientStats",
    "createTelegramTab",
    "onBotClientCreated",
    "onBotClientRevoked",
    "syncBotWithSettings",
    "createUrlHistoryTab",
    "addUrlAccess",
    "UrlAccess",
    "access",
    "updateUrlTable",
    "diagnoseBotConnection",
    "createMTProxyTab",
    "onMTProxyStarted",
    "onMTProxyStopped",
    "onMTProxyLog",
    "msg",
    "level"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      99,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  608,    2, 0x06,    1 /* Public */,
       3,    0,  609,    2, 0x06,    2 /* Public */,
       4,    1,  610,    2, 0x06,    3 /* Public */,
       6,    1,  613,    2, 0x06,    5 /* Public */,
       9,    0,  616,    2, 0x06,    7 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      10,    0,  617,    2, 0x0a,    8 /* Public */,
      11,    0,  618,    2, 0x0a,    9 /* Public */,
      12,    0,  619,    2, 0x10a,   10 /* Public | MethodIsConst  */,
      13,    2,  620,    2, 0x0a,   11 /* Public */,
      17,    0,  625,    2, 0x0a,   14 /* Public */,
      18,    0,  626,    2, 0x0a,   15 /* Public */,
      19,    0,  627,    2, 0x08,   16 /* Private */,
      20,    0,  628,    2, 0x08,   17 /* Private */,
      21,    0,  629,    2, 0x08,   18 /* Private */,
      22,    2,  630,    2, 0x08,   19 /* Private */,
      26,    1,  635,    2, 0x08,   22 /* Private */,
      29,    0,  638,    2, 0x08,   24 /* Private */,
      30,    0,  639,    2, 0x08,   25 /* Private */,
      31,    1,  640,    2, 0x08,   26 /* Private */,
      33,    1,  643,    2, 0x08,   28 /* Private */,
      35,    0,  646,    2, 0x08,   30 /* Private */,
      36,    0,  647,    2, 0x08,   31 /* Private */,
      37,    0,  648,    2, 0x08,   32 /* Private */,
      38,    0,  649,    2, 0x08,   33 /* Private */,
      39,    0,  650,    2, 0x08,   34 /* Private */,
      40,    0,  651,    2, 0x08,   35 /* Private */,
      41,    2,  652,    2, 0x08,   36 /* Private */,
      42,    1,  657,    2, 0x08,   39 /* Private */,
      43,    0,  660,    2, 0x08,   41 /* Private */,
      44,    0,  661,    2, 0x08,   42 /* Private */,
      45,    0,  662,    2, 0x08,   43 /* Private */,
      46,    1,  663,    2, 0x08,   44 /* Private */,
      48,    0,  666,    2, 0x08,   46 /* Private */,
      49,    0,  667,    2, 0x08,   47 /* Private */,
      50,    0,  668,    2, 0x08,   48 /* Private */,
      51,    1,  669,    2, 0x08,   49 /* Private */,
      53,    0,  672,    2, 0x08,   51 /* Private */,
      54,    0,  673,    2, 0x08,   52 /* Private */,
      55,    0,  674,    2, 0x08,   53 /* Private */,
      56,    0,  675,    2, 0x08,   54 /* Private */,
      57,    0,  676,    2, 0x08,   55 /* Private */,
      58,    1,  677,    2, 0x08,   56 /* Private */,
      61,    0,  680,    2, 0x08,   58 /* Private */,
      62,    0,  681,    2, 0x08,   59 /* Private */,
      63,    2,  682,    2, 0x08,   60 /* Private */,
      63,    1,  687,    2, 0x28,   63 /* Private | MethodCloned */,
      66,    0,  690,    2, 0x08,   65 /* Private */,
      67,    0,  691,    2, 0x08,   66 /* Private */,
      68,    0,  692,    2, 0x08,   67 /* Private */,
      69,    0,  693,    2, 0x08,   68 /* Private */,
      70,    0,  694,    2, 0x08,   69 /* Private */,
      71,    1,  695,    2, 0x08,   70 /* Private */,
      73,    0,  698,    2, 0x08,   72 /* Private */,
      74,    0,  699,    2, 0x08,   73 /* Private */,
      75,    0,  700,    2, 0x08,   74 /* Private */,
      76,    1,  701,    2, 0x08,   75 /* Private */,
      78,    0,  704,    2, 0x08,   77 /* Private */,
      79,    0,  705,    2, 0x08,   78 /* Private */,
      80,    0,  706,    2, 0x08,   79 /* Private */,
      81,    1,  707,    2, 0x08,   80 /* Private */,
      82,    0,  710,    2, 0x08,   82 /* Private */,
      83,    0,  711,    2, 0x08,   83 /* Private */,
      84,    0,  712,    2, 0x08,   84 /* Private */,
      85,    0,  713,    2, 0x08,   85 /* Private */,
      86,    0,  714,    2, 0x08,   86 /* Private */,
      87,    0,  715,    2, 0x08,   87 /* Private */,
      88,    2,  716,    2, 0x08,   88 /* Private */,
      92,    0,  721,    2, 0x08,   91 /* Private */,
      93,    0,  722,    2, 0x08,   92 /* Private */,
      94,    0,  723,    2, 0x08,   93 /* Private */,
      95,    0,  724,    2, 0x08,   94 /* Private */,
      96,    2,  725,    2, 0x08,   95 /* Private */,
      98,    0,  730,    2, 0x08,   98 /* Private */,
      99,    0,  731,    2, 0x08,   99 /* Private */,
     100,    1,  732,    2, 0x08,  100 /* Private */,
     102,    0,  735,    2, 0x08,  102 /* Private */,
     103,    0,  736,    2, 0x08,  103 /* Private */,
     104,    0,  737,    2, 0x08,  104 /* Private */,
     105,    0,  738,    2, 0x08,  105 /* Private */,
     106,    0,  739,    2, 0x08,  106 /* Private */,
     107,    0,  740,    2, 0x08,  107 /* Private */,
     108,    0,  741,    2, 0x08,  108 /* Private */,
     109,    0,  742,    2, 0x08,  109 /* Private */,
     110,    0,  743,    2, 0x08,  110 /* Private */,
     111,    0,  744,    2, 0x08,  111 /* Private */,
     112,    1,  745,    2, 0x08,  112 /* Private */,
     113,    0,  748,    2, 0x08,  114 /* Private */,
     114,    0,  749,    2, 0x08,  115 /* Private */,
     115,    1,  750,    2, 0x08,  116 /* Private */,
     116,    1,  753,    2, 0x08,  118 /* Private */,
     117,    0,  756,    2, 0x08,  120 /* Private */,
     118,    0,  757,    2, 0x08,  121 /* Private */,
     119,    1,  758,    2, 0x08,  122 /* Private */,
     122,    0,  761,    2, 0x08,  124 /* Private */,
     123,    0,  762,    2, 0x08,  125 /* Private */,
     124,    0,  763,    2, 0x08,  126 /* Private */,
     125,    0,  764,    2, 0x08,  127 /* Private */,
     126,    0,  765,    2, 0x08,  128 /* Private */,
     127,    2,  766,    2, 0x08,  129 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 15,   14,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 24,   23,   25,
    QMetaType::Void, 0x80000000 | 27,   28,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   32,
    QMetaType::Void, QMetaType::QString,   34,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 24,   23,   25,
    QMetaType::Void, 0x80000000 | 27,   28,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   47,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   52,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 59,   60,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   64,   65,
    QMetaType::Void, QMetaType::QString,   64,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   72,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   77,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QString,
    QMetaType::Bool, QMetaType::Bool,   77,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 89, QMetaType::QDateTime,   90,   91,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   14,   97,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,  101,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 120,  121,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,  128,  129,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSizes,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'serverStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clientsUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'torStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'registryUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QMap<QString,ClientRecord> &, std::false_type>,
        // method 'clientRegistryChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startTor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startOpenVPNServer'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'getClientRegistry'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateRegistryFromTelegram'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ClientRecord &, std::false_type>,
        // method 'startMTProxy'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopMTProxy'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopTor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'restartTor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTorStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTorFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>,
        // method 'onTorError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>,
        // method 'onTorReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkTorStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'parseTorCircuitLine'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendTorCommand'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onControlSocketConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onControlSocketReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onControlSocketError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'requestNewCircuit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopOpenVPNServer'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onServerStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onServerFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>,
        // method 'onServerError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>,
        // method 'onServerReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'generateCertificates'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'generateCertificatesAsync'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCertGenerationFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'checkCertificates'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'generateClientConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'generateTestAndroidConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'generateClientCertificate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'checkIPLeak'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onIPCheckFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showSettings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showAbout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTrayActivated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSystemTrayIcon::ActivationReason, std::false_type>,
        // method 'applySettings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateTrafficStats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addLogMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addLogMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'flushLogs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addBridge'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'removeBridge'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'importBridgesFromText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'validateBridgeFormat'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'testBridgeConnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateBridgeConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'enableKillSwitch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disableKillSwitch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setupFirewallRules'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'diagnoseConnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'testServerConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'getExternalInterface'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'setupIPTablesRules'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'applyRoutingManually'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'verifyRouting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'enableIPForwarding'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkIPForwarding'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'createClientsTab'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateClientsTable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateClientsTableUI'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QMap<QString,ClientInfo> &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QDateTime &, std::false_type>,
        // method 'disconnectSelectedClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disconnectAllClients'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showClientDetails'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'banClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'applySpeedLimit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'exportClientsLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearClientsLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onClientTableContextMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>,
        // method 'refreshClientsNow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'generateNamedClientConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showClientAnalytics'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showClientSessionHistory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadClientRegistry'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveClientRegistry'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateRegistryTable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'applyRegistryFilter'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateRealtimeDurations'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkMultipleConnections'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'deleteClientPermanently'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateClientStats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createTelegramTab'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBotClientCreated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onBotClientRevoked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'syncBotWithSettings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createUrlHistoryTab'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addUrlAccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const UrlAccess &, std::false_type>,
        // method 'updateUrlTable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'diagnoseBotConnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'createMTProxyTab'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMTProxyStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMTProxyStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMTProxyLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->serverStarted(); break;
        case 1: _t->clientsUpdated(); break;
        case 2: _t->torStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->registryUpdated((*reinterpret_cast< std::add_pointer_t<QMap<QString,ClientRecord>>>(_a[1]))); break;
        case 4: _t->clientRegistryChanged(); break;
        case 5: _t->startTor(); break;
        case 6: _t->startOpenVPNServer(); break;
        case 7: _t->getClientRegistry(); break;
        case 8: _t->updateRegistryFromTelegram((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ClientRecord>>(_a[2]))); break;
        case 9: _t->startMTProxy(); break;
        case 10: _t->stopMTProxy(); break;
        case 11: _t->stopTor(); break;
        case 12: _t->restartTor(); break;
        case 13: _t->onTorStarted(); break;
        case 14: _t->onTorFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 15: _t->onTorError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 16: _t->onTorReadyRead(); break;
        case 17: _t->checkTorStatus(); break;
        case 18: _t->parseTorCircuitLine((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->sendTorCommand((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->onControlSocketConnected(); break;
        case 21: _t->onControlSocketReadyRead(); break;
        case 22: _t->onControlSocketError(); break;
        case 23: _t->requestNewCircuit(); break;
        case 24: _t->stopOpenVPNServer(); break;
        case 25: _t->onServerStarted(); break;
        case 26: _t->onServerFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 27: _t->onServerError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 28: _t->onServerReadyRead(); break;
        case 29: _t->generateCertificates(); break;
        case 30: _t->generateCertificatesAsync(); break;
        case 31: _t->onCertGenerationFinished((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 32: _t->checkCertificates(); break;
        case 33: _t->generateClientConfig(); break;
        case 34: _t->generateTestAndroidConfig(); break;
        case 35: _t->generateClientCertificate((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 36: _t->checkIPLeak(); break;
        case 37: _t->onIPCheckFinished(); break;
        case 38: _t->updateStatus(); break;
        case 39: _t->showSettings(); break;
        case 40: _t->showAbout(); break;
        case 41: _t->onTrayActivated((*reinterpret_cast< std::add_pointer_t<QSystemTrayIcon::ActivationReason>>(_a[1]))); break;
        case 42: _t->applySettings(); break;
        case 43: _t->updateTrafficStats(); break;
        case 44: _t->addLogMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 45: _t->addLogMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 46: _t->flushLogs(); break;
        case 47: _t->addBridge(); break;
        case 48: _t->removeBridge(); break;
        case 49: _t->importBridgesFromText(); break;
        case 50: _t->validateBridgeFormat(); break;
        case 51: _t->testBridgeConnection((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 52: _t->updateBridgeConfig(); break;
        case 53: _t->enableKillSwitch(); break;
        case 54: _t->disableKillSwitch(); break;
        case 55: _t->setupFirewallRules((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 56: _t->diagnoseConnection(); break;
        case 57: _t->testServerConfig(); break;
        case 58: { QString _r = _t->getExternalInterface();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 59: { bool _r = _t->setupIPTablesRules((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 60: _t->applyRoutingManually(); break;
        case 61: _t->verifyRouting(); break;
        case 62: _t->enableIPForwarding(); break;
        case 63: { bool _r = _t->checkIPForwarding();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 64: _t->createClientsTab(); break;
        case 65: _t->updateClientsTable(); break;
        case 66: _t->updateClientsTableUI((*reinterpret_cast< std::add_pointer_t<QMap<QString,ClientInfo>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QDateTime>>(_a[2]))); break;
        case 67: _t->disconnectSelectedClient(); break;
        case 68: _t->disconnectAllClients(); break;
        case 69: _t->showClientDetails(); break;
        case 70: _t->banClient(); break;
        case 71: _t->applySpeedLimit((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 72: _t->exportClientsLog(); break;
        case 73: _t->clearClientsLog(); break;
        case 74: _t->onClientTableContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 75: _t->refreshClientsNow(); break;
        case 76: _t->generateNamedClientConfig(); break;
        case 77: _t->showClientAnalytics(); break;
        case 78: _t->showClientSessionHistory(); break;
        case 79: _t->loadClientRegistry(); break;
        case 80: _t->saveClientRegistry(); break;
        case 81: _t->updateRegistryTable(); break;
        case 82: _t->applyRegistryFilter(); break;
        case 83: _t->updateRealtimeDurations(); break;
        case 84: _t->checkMultipleConnections(); break;
        case 85: _t->deleteClientPermanently((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 86: _t->updateClientStats(); break;
        case 87: _t->createTelegramTab(); break;
        case 88: _t->onBotClientCreated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 89: _t->onBotClientRevoked((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 90: _t->syncBotWithSettings(); break;
        case 91: _t->createUrlHistoryTab(); break;
        case 92: _t->addUrlAccess((*reinterpret_cast< std::add_pointer_t<UrlAccess>>(_a[1]))); break;
        case 93: _t->updateUrlTable(); break;
        case 94: _t->diagnoseBotConnection(); break;
        case 95: _t->createMTProxyTab(); break;
        case 96: _t->onMTProxyStarted(); break;
        case 97: _t->onMTProxyStopped(); break;
        case 98: _t->onMTProxyLog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MainWindow::*)();
            if (_t _q_method = &MainWindow::serverStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (_t _q_method = &MainWindow::clientsUpdated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(bool );
            if (_t _q_method = &MainWindow::torStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(const QMap<QString,ClientRecord> & );
            if (_t _q_method = &MainWindow::registryUpdated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)();
            if (_t _q_method = &MainWindow::clientRegistryChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 99)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 99;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 99)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 99;
    }
    return _id;
}

// SIGNAL 0
void MainWindow::serverStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MainWindow::clientsUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MainWindow::torStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void MainWindow::registryUpdated(const QMap<QString,ClientRecord> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void MainWindow::clientRegistryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
