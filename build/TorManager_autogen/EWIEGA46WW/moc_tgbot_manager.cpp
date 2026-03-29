/****************************************************************************
** Meta object code from reading C++ file 'tgbot_manager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../tgbot_manager.h"
#include <QtNetwork/QSslError>
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tgbot_manager.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_TelegramBotManager_t {
    uint offsetsAndSizes[206];
    char stringdata0[19];
    char stringdata1[11];
    char stringdata2[1];
    char stringdata3[4];
    char stringdata4[6];
    char stringdata5[14];
    char stringdata6[8];
    char stringdata7[14];
    char stringdata8[3];
    char stringdata9[14];
    char stringdata10[15];
    char stringdata11[12];
    char stringdata12[13];
    char stringdata13[12];
    char stringdata14[18];
    char stringdata15[27];
    char stringdata16[9];
    char stringdata17[7];
    char stringdata18[12];
    char stringdata19[15];
    char stringdata20[6];
    char stringdata21[12];
    char stringdata22[7];
    char stringdata23[5];
    char stringdata24[15];
    char stringdata25[10];
    char stringdata26[12];
    char stringdata27[10];
    char stringdata28[20];
    char stringdata29[16];
    char stringdata30[13];
    char stringdata31[9];
    char stringdata32[8];
    char stringdata33[14];
    char stringdata34[7];
    char stringdata35[15];
    char stringdata36[10];
    char stringdata37[16];
    char stringdata38[12];
    char stringdata39[11];
    char stringdata40[13];
    char stringdata41[11];
    char stringdata42[14];
    char stringdata43[13];
    char stringdata44[16];
    char stringdata45[18];
    char stringdata46[13];
    char stringdata47[25];
    char stringdata48[19];
    char stringdata49[18];
    char stringdata50[13];
    char stringdata51[4];
    char stringdata52[19];
    char stringdata53[12];
    char stringdata54[12];
    char stringdata55[17];
    char stringdata56[18];
    char stringdata57[14];
    char stringdata58[12];
    char stringdata59[18];
    char stringdata60[11];
    char stringdata61[27];
    char stringdata62[21];
    char stringdata63[16];
    char stringdata64[7];
    char stringdata65[20];
    char stringdata66[20];
    char stringdata67[7];
    char stringdata68[18];
    char stringdata69[13];
    char stringdata70[29];
    char stringdata71[13];
    char stringdata72[8];
    char stringdata73[11];
    char stringdata74[16];
    char stringdata75[4];
    char stringdata76[23];
    char stringdata77[7];
    char stringdata78[24];
    char stringdata79[10];
    char stringdata80[10];
    char stringdata81[12];
    char stringdata82[12];
    char stringdata83[19];
    char stringdata84[8];
    char stringdata85[19];
    char stringdata86[16];
    char stringdata87[8];
    char stringdata88[8];
    char stringdata89[11];
    char stringdata90[15];
    char stringdata91[18];
    char stringdata92[15];
    char stringdata93[14];
    char stringdata94[17];
    char stringdata95[19];
    char stringdata96[14];
    char stringdata97[24];
    char stringdata98[14];
    char stringdata99[7];
    char stringdata100[18];
    char stringdata101[19];
    char stringdata102[18];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_TelegramBotManager_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_TelegramBotManager_t qt_meta_stringdata_TelegramBotManager = {
    {
        QT_MOC_LITERAL(0, 18),  // "TelegramBotManager"
        QT_MOC_LITERAL(19, 10),  // "logMessage"
        QT_MOC_LITERAL(30, 0),  // ""
        QT_MOC_LITERAL(31, 3),  // "msg"
        QT_MOC_LITERAL(35, 5),  // "level"
        QT_MOC_LITERAL(41, 13),  // "statusChanged"
        QT_MOC_LITERAL(55, 7),  // "running"
        QT_MOC_LITERAL(63, 13),  // "clientCreated"
        QT_MOC_LITERAL(77, 2),  // "cn"
        QT_MOC_LITERAL(80, 13),  // "clientRevoked"
        QT_MOC_LITERAL(94, 14),  // "clientsChanged"
        QT_MOC_LITERAL(109, 11),  // "caGenerated"
        QT_MOC_LITERAL(121, 12),  // "startPolling"
        QT_MOC_LITERAL(134, 11),  // "stopPolling"
        QT_MOC_LITERAL(146, 17),  // "onRegistryUpdated"
        QT_MOC_LITERAL(164, 26),  // "QMap<QString,ClientRecord>"
        QT_MOC_LITERAL(191, 8),  // "registry"
        QT_MOC_LITERAL(200, 6),  // "doPoll"
        QT_MOC_LITERAL(207, 11),  // "onPollReply"
        QT_MOC_LITERAL(219, 14),  // "QNetworkReply*"
        QT_MOC_LITERAL(234, 5),  // "reply"
        QT_MOC_LITERAL(240, 11),  // "sendMessage"
        QT_MOC_LITERAL(252, 6),  // "chatId"
        QT_MOC_LITERAL(259, 4),  // "text"
        QT_MOC_LITERAL(264, 14),  // "inlineKeyboard"
        QT_MOC_LITERAL(279, 9),  // "parseMode"
        QT_MOC_LITERAL(289, 11),  // "editMessage"
        QT_MOC_LITERAL(301, 9),  // "messageId"
        QT_MOC_LITERAL(311, 19),  // "answerCallbackQuery"
        QT_MOC_LITERAL(331, 15),  // "callbackQueryId"
        QT_MOC_LITERAL(347, 12),  // "sendDocument"
        QT_MOC_LITERAL(360, 8),  // "filePath"
        QT_MOC_LITERAL(369, 7),  // "caption"
        QT_MOC_LITERAL(377, 13),  // "processUpdate"
        QT_MOC_LITERAL(391, 6),  // "update"
        QT_MOC_LITERAL(398, 14),  // "processMessage"
        QT_MOC_LITERAL(413, 9),  // "TgMessage"
        QT_MOC_LITERAL(423, 15),  // "processCallback"
        QT_MOC_LITERAL(439, 11),  // "handleStart"
        QT_MOC_LITERAL(451, 10),  // "handleHelp"
        QT_MOC_LITERAL(462, 12),  // "handleStatus"
        QT_MOC_LITERAL(475, 10),  // "handleList"
        QT_MOC_LITERAL(486, 13),  // "handleNewUser"
        QT_MOC_LITERAL(500, 12),  // "handleRevoke"
        QT_MOC_LITERAL(513, 15),  // "handleGetConfig"
        QT_MOC_LITERAL(529, 17),  // "handleInstruction"
        QT_MOC_LITERAL(547, 12),  // "handleCancel"
        QT_MOC_LITERAL(560, 24),  // "handleNewUserForNonAdmin"
        QT_MOC_LITERAL(585, 18),  // "sendExistingConfig"
        QT_MOC_LITERAL(604, 17),  // "handleConvMessage"
        QT_MOC_LITERAL(622, 12),  // "ConvContext&"
        QT_MOC_LITERAL(635, 3),  // "ctx"
        QT_MOC_LITERAL(639, 18),  // "handleConvCallback"
        QT_MOC_LITERAL(658, 11),  // "convConsent"
        QT_MOC_LITERAL(670, 11),  // "convGotName"
        QT_MOC_LITERAL(682, 16),  // "convExpiryChoice"
        QT_MOC_LITERAL(699, 17),  // "convGotExpiryDate"
        QT_MOC_LITERAL(717, 13),  // "convDoConfirm"
        QT_MOC_LITERAL(731, 11),  // "convConfirm"
        QT_MOC_LITERAL(743, 17),  // "convRevokeConfirm"
        QT_MOC_LITERAL(761, 10),  // "generateCA"
        QT_MOC_LITERAL(772, 26),  // "generateClientCertDetailed"
        QT_MOC_LITERAL(799, 20),  // "CertGenerationResult"
        QT_MOC_LITERAL(820, 15),  // "buildOvpnConfig"
        QT_MOC_LITERAL(836, 6),  // "expiry"
        QT_MOC_LITERAL(843, 19),  // "writeRegistryClient"
        QT_MOC_LITERAL(863, 19),  // "banClientInRegistry"
        QT_MOC_LITERAL(883, 6),  // "banned"
        QT_MOC_LITERAL(890, 17),  // "revokeClientCerts"
        QT_MOC_LITERAL(908, 12),  // "readRegistry"
        QT_MOC_LITERAL(921, 28),  // "QMap<QString,TgClientRecord>"
        QT_MOC_LITERAL(950, 12),  // "saveRegistry"
        QT_MOC_LITERAL(963, 7),  // "clients"
        QT_MOC_LITERAL(971, 10),  // "saveClient"
        QT_MOC_LITERAL(982, 15),  // "TgClientRecord&"
        QT_MOC_LITERAL(998, 3),  // "rec"
        QT_MOC_LITERAL(1002, 22),  // "logCertGenerationError"
        QT_MOC_LITERAL(1025, 6),  // "result"
        QT_MOC_LITERAL(1032, 23),  // "generateClientCertAsync"
        QT_MOC_LITERAL(1056, 9),  // "hasExpiry"
        QT_MOC_LITERAL(1066, 9),  // "editMsgId"
        QT_MOC_LITERAL(1076, 11),  // "isAdminFlow"
        QT_MOC_LITERAL(1088, 11),  // "originalMsg"
        QT_MOC_LITERAL(1100, 18),  // "handleNetworkError"
        QT_MOC_LITERAL(1119, 7),  // "context"
        QT_MOC_LITERAL(1127, 18),  // "retryFailedRequest"
        QT_MOC_LITERAL(1146, 15),  // "QNetworkRequest"
        QT_MOC_LITERAL(1162, 7),  // "request"
        QT_MOC_LITERAL(1170, 7),  // "payload"
        QT_MOC_LITERAL(1178, 10),  // "retryCount"
        QT_MOC_LITERAL(1189, 14),  // "onBtnStartStop"
        QT_MOC_LITERAL(1204, 17),  // "onBtnSaveSettings"
        QT_MOC_LITERAL(1222, 14),  // "onBtnTestToken"
        QT_MOC_LITERAL(1237, 13),  // "onBtnAddAdmin"
        QT_MOC_LITERAL(1251, 16),  // "onBtnRemoveAdmin"
        QT_MOC_LITERAL(1268, 18),  // "onBtnSendBroadcast"
        QT_MOC_LITERAL(1287, 13),  // "onBtnClearLog"
        QT_MOC_LITERAL(1301, 23),  // "onTokenVisibilityToggle"
        QT_MOC_LITERAL(1325, 13),  // "onPollTimeout"
        QT_MOC_LITERAL(1339, 6),  // "addLog"
        QT_MOC_LITERAL(1346, 17),  // "updateStatusLabel"
        QT_MOC_LITERAL(1364, 18),  // "updateClientsTable"
        QT_MOC_LITERAL(1383, 17)   // "refreshAdminsList"
    },
    "TelegramBotManager",
    "logMessage",
    "",
    "msg",
    "level",
    "statusChanged",
    "running",
    "clientCreated",
    "cn",
    "clientRevoked",
    "clientsChanged",
    "caGenerated",
    "startPolling",
    "stopPolling",
    "onRegistryUpdated",
    "QMap<QString,ClientRecord>",
    "registry",
    "doPoll",
    "onPollReply",
    "QNetworkReply*",
    "reply",
    "sendMessage",
    "chatId",
    "text",
    "inlineKeyboard",
    "parseMode",
    "editMessage",
    "messageId",
    "answerCallbackQuery",
    "callbackQueryId",
    "sendDocument",
    "filePath",
    "caption",
    "processUpdate",
    "update",
    "processMessage",
    "TgMessage",
    "processCallback",
    "handleStart",
    "handleHelp",
    "handleStatus",
    "handleList",
    "handleNewUser",
    "handleRevoke",
    "handleGetConfig",
    "handleInstruction",
    "handleCancel",
    "handleNewUserForNonAdmin",
    "sendExistingConfig",
    "handleConvMessage",
    "ConvContext&",
    "ctx",
    "handleConvCallback",
    "convConsent",
    "convGotName",
    "convExpiryChoice",
    "convGotExpiryDate",
    "convDoConfirm",
    "convConfirm",
    "convRevokeConfirm",
    "generateCA",
    "generateClientCertDetailed",
    "CertGenerationResult",
    "buildOvpnConfig",
    "expiry",
    "writeRegistryClient",
    "banClientInRegistry",
    "banned",
    "revokeClientCerts",
    "readRegistry",
    "QMap<QString,TgClientRecord>",
    "saveRegistry",
    "clients",
    "saveClient",
    "TgClientRecord&",
    "rec",
    "logCertGenerationError",
    "result",
    "generateClientCertAsync",
    "hasExpiry",
    "editMsgId",
    "isAdminFlow",
    "originalMsg",
    "handleNetworkError",
    "context",
    "retryFailedRequest",
    "QNetworkRequest",
    "request",
    "payload",
    "retryCount",
    "onBtnStartStop",
    "onBtnSaveSettings",
    "onBtnTestToken",
    "onBtnAddAdmin",
    "onBtnRemoveAdmin",
    "onBtnSendBroadcast",
    "onBtnClearLog",
    "onTokenVisibilityToggle",
    "onPollTimeout",
    "addLog",
    "updateStatusLabel",
    "updateClientsTable",
    "refreshAdminsList"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_TelegramBotManager[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      73,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  452,    2, 0x06,    1 /* Public */,
       1,    1,  457,    2, 0x26,    4 /* Public | MethodCloned */,
       5,    1,  460,    2, 0x06,    6 /* Public */,
       7,    1,  463,    2, 0x06,    8 /* Public */,
       9,    1,  466,    2, 0x06,   10 /* Public */,
      10,    0,  469,    2, 0x06,   12 /* Public */,
      11,    0,  470,    2, 0x06,   13 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    0,  471,    2, 0x0a,   14 /* Public */,
      13,    0,  472,    2, 0x0a,   15 /* Public */,
      14,    1,  473,    2, 0x0a,   16 /* Public */,
      17,    0,  476,    2, 0x08,   18 /* Private */,
      18,    1,  477,    2, 0x08,   19 /* Private */,
      21,    4,  480,    2, 0x08,   21 /* Private */,
      21,    3,  489,    2, 0x28,   26 /* Private | MethodCloned */,
      21,    2,  496,    2, 0x28,   30 /* Private | MethodCloned */,
      26,    5,  501,    2, 0x08,   33 /* Private */,
      26,    4,  512,    2, 0x28,   39 /* Private | MethodCloned */,
      26,    3,  521,    2, 0x28,   44 /* Private | MethodCloned */,
      28,    2,  528,    2, 0x08,   48 /* Private */,
      28,    1,  533,    2, 0x28,   51 /* Private | MethodCloned */,
      30,    4,  536,    2, 0x08,   53 /* Private */,
      30,    3,  545,    2, 0x28,   58 /* Private | MethodCloned */,
      30,    2,  552,    2, 0x28,   62 /* Private | MethodCloned */,
      33,    1,  557,    2, 0x08,   65 /* Private */,
      35,    1,  560,    2, 0x08,   67 /* Private */,
      37,    1,  563,    2, 0x08,   69 /* Private */,
      38,    1,  566,    2, 0x08,   71 /* Private */,
      39,    1,  569,    2, 0x08,   73 /* Private */,
      40,    1,  572,    2, 0x08,   75 /* Private */,
      41,    1,  575,    2, 0x08,   77 /* Private */,
      42,    1,  578,    2, 0x08,   79 /* Private */,
      43,    1,  581,    2, 0x08,   81 /* Private */,
      44,    1,  584,    2, 0x08,   83 /* Private */,
      45,    1,  587,    2, 0x08,   85 /* Private */,
      46,    1,  590,    2, 0x08,   87 /* Private */,
      47,    1,  593,    2, 0x08,   89 /* Private */,
      48,    2,  596,    2, 0x08,   91 /* Private */,
      49,    2,  601,    2, 0x08,   94 /* Private */,
      52,    2,  606,    2, 0x08,   97 /* Private */,
      53,    2,  611,    2, 0x08,  100 /* Private */,
      54,    2,  616,    2, 0x08,  103 /* Private */,
      55,    2,  621,    2, 0x08,  106 /* Private */,
      56,    2,  626,    2, 0x08,  109 /* Private */,
      57,    2,  631,    2, 0x08,  112 /* Private */,
      58,    2,  636,    2, 0x08,  115 /* Private */,
      59,    2,  641,    2, 0x08,  118 /* Private */,
      60,    0,  646,    2, 0x08,  121 /* Private */,
      61,    1,  647,    2, 0x08,  122 /* Private */,
      63,    2,  650,    2, 0x08,  124 /* Private */,
      65,    2,  655,    2, 0x08,  127 /* Private */,
      66,    2,  660,    2, 0x08,  130 /* Private */,
      68,    1,  665,    2, 0x08,  133 /* Private */,
      69,    0,  668,    2, 0x08,  135 /* Private */,
      71,    1,  669,    2, 0x08,  136 /* Private */,
      73,    2,  672,    2, 0x08,  138 /* Private */,
      76,    2,  677,    2, 0x08,  141 /* Private */,
      78,    7,  682,    2, 0x08,  144 /* Private */,
      83,    2,  697,    2, 0x08,  152 /* Private */,
      85,    5,  702,    2, 0x08,  155 /* Private */,
      90,    0,  713,    2, 0x08,  161 /* Private */,
      91,    0,  714,    2, 0x08,  162 /* Private */,
      92,    0,  715,    2, 0x08,  163 /* Private */,
      93,    0,  716,    2, 0x08,  164 /* Private */,
      94,    0,  717,    2, 0x08,  165 /* Private */,
      95,    0,  718,    2, 0x08,  166 /* Private */,
      96,    0,  719,    2, 0x08,  167 /* Private */,
      97,    0,  720,    2, 0x08,  168 /* Private */,
      98,    0,  721,    2, 0x08,  169 /* Private */,
      99,    2,  722,    2, 0x08,  170 /* Private */,
      99,    1,  727,    2, 0x28,  173 /* Private | MethodCloned */,
     100,    0,  730,    2, 0x08,  175 /* Private */,
     101,    0,  731,    2, 0x08,  176 /* Private */,
     102,    0,  732,    2, 0x08,  177 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QJsonArray, QMetaType::QString,   22,   23,   24,   25,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QJsonArray,   22,   23,   24,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,   22,   23,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Int, QMetaType::QString, QMetaType::QJsonArray, QMetaType::QString,   22,   27,   23,   24,   25,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Int, QMetaType::QString, QMetaType::QJsonArray,   22,   27,   23,   24,
    QMetaType::Void, QMetaType::LongLong, QMetaType::Int, QMetaType::QString,   22,   27,   23,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   29,   23,
    QMetaType::Void, QMetaType::QString,   29,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::QString,   22,   31,   32,   25,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString, QMetaType::QString,   22,   31,   32,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,   22,   31,
    QMetaType::Void, QMetaType::QJsonObject,   34,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36,    3,
    QMetaType::Void, 0x80000000 | 36, QMetaType::QString,    3,    8,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Void, 0x80000000 | 36, 0x80000000 | 50,    3,   51,
    QMetaType::Bool,
    0x80000000 | 62, QMetaType::QString,    8,
    QMetaType::QString, QMetaType::QString, QMetaType::QDate,    8,   64,
    QMetaType::Bool, QMetaType::QString, QMetaType::QDate,    8,   64,
    QMetaType::Bool, QMetaType::QString, QMetaType::Bool,    8,   67,
    QMetaType::Bool, QMetaType::QString,    8,
    0x80000000 | 70,
    QMetaType::Void, 0x80000000 | 70,   72,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 74,    8,   75,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 62,    8,   77,
    QMetaType::Void, QMetaType::QString, QMetaType::QDate, QMetaType::Bool, QMetaType::LongLong, QMetaType::Int, QMetaType::Bool, 0x80000000 | 36,    8,   64,   79,   22,   80,   81,   82,
    QMetaType::Void, 0x80000000 | 19, QMetaType::QString,   20,   84,
    QMetaType::Void, 0x80000000 | 86, QMetaType::QByteArray, QMetaType::Int, QMetaType::LongLong, QMetaType::QString,   87,   88,   89,   22,   84,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject TelegramBotManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_TelegramBotManager.offsetsAndSizes,
    qt_meta_data_TelegramBotManager,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_TelegramBotManager_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TelegramBotManager, std::true_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'statusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'clientCreated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clientRevoked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clientsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'caGenerated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startPolling'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopPolling'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRegistryUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QMap<QString,ClientRecord> &, std::false_type>,
        // method 'doPoll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPollReply'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        // method 'sendMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonArray &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonArray &, std::false_type>,
        // method 'sendMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'editMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonArray &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'editMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonArray &, std::false_type>,
        // method 'editMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'answerCallbackQuery'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'answerCallbackQuery'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendDocument'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendDocument'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendDocument'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'processUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>,
        // method 'processMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'processCallback'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleHelp'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleList'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleNewUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleRevoke'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleGetConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleInstruction'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleCancel'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleNewUserForNonAdmin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'sendExistingConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'handleConvMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'handleConvCallback'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'convConsent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'convGotName'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'convExpiryChoice'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'convGotExpiryDate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'convDoConfirm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'convConfirm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'convRevokeConfirm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        QtPrivate::TypeAndForceComplete<ConvContext &, std::false_type>,
        // method 'generateCA'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'generateClientCertDetailed'
        QtPrivate::TypeAndForceComplete<CertGenerationResult, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'buildOvpnConfig'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QDate &, std::false_type>,
        // method 'writeRegistryClient'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QDate &, std::false_type>,
        // method 'banClientInRegistry'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'revokeClientCerts'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'readRegistry'
        QtPrivate::TypeAndForceComplete<QMap<QString,TgClientRecord>, std::false_type>,
        // method 'saveRegistry'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QMap<QString,TgClientRecord> &, std::false_type>,
        // method 'saveClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<TgClientRecord &, std::false_type>,
        // method 'logCertGenerationError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const CertGenerationResult &, std::false_type>,
        // method 'generateClientCertAsync'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QDate &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const TgMessage &, std::false_type>,
        // method 'handleNetworkError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'retryFailedRequest'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkRequest, std::false_type>,
        QtPrivate::TypeAndForceComplete<QByteArray, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onBtnStartStop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnSaveSettings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnTestToken'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnAddAdmin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnRemoveAdmin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnSendBroadcast'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBtnClearLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTokenVisibilityToggle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPollTimeout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateStatusLabel'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateClientsTable'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshAdminsList'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void TelegramBotManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TelegramBotManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->logMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->logMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->statusChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->clientCreated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->clientRevoked((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->clientsChanged(); break;
        case 6: _t->caGenerated(); break;
        case 7: _t->startPolling(); break;
        case 8: _t->stopPolling(); break;
        case 9: _t->onRegistryUpdated((*reinterpret_cast< std::add_pointer_t<QMap<QString,ClientRecord>>>(_a[1]))); break;
        case 10: _t->doPoll(); break;
        case 11: _t->onPollReply((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 12: _t->sendMessage((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QJsonArray>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 13: _t->sendMessage((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QJsonArray>>(_a[3]))); break;
        case 14: _t->sendMessage((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 15: _t->editMessage((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QJsonArray>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[5]))); break;
        case 16: _t->editMessage((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QJsonArray>>(_a[4]))); break;
        case 17: _t->editMessage((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 18: _t->answerCallbackQuery((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 19: _t->answerCallbackQuery((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->sendDocument((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 21: _t->sendDocument((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 22: _t->sendDocument((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 23: _t->processUpdate((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 24: _t->processMessage((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 25: _t->processCallback((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 26: _t->handleStart((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 27: _t->handleHelp((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 28: _t->handleStatus((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 29: _t->handleList((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 30: _t->handleNewUser((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 31: _t->handleRevoke((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 32: _t->handleGetConfig((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 33: _t->handleInstruction((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 34: _t->handleCancel((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 35: _t->handleNewUserForNonAdmin((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1]))); break;
        case 36: _t->sendExistingConfig((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 37: _t->handleConvMessage((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 38: _t->handleConvCallback((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 39: _t->convConsent((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 40: _t->convGotName((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 41: _t->convExpiryChoice((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 42: _t->convGotExpiryDate((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 43: _t->convDoConfirm((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 44: _t->convConfirm((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 45: _t->convRevokeConfirm((*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ConvContext&>>(_a[2]))); break;
        case 46: { bool _r = _t->generateCA();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 47: { CertGenerationResult _r = _t->generateClientCertDetailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< CertGenerationResult*>(_a[0]) = std::move(_r); }  break;
        case 48: { QString _r = _t->buildOvpnConfig((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QDate>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 49: { bool _r = _t->writeRegistryClient((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QDate>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 50: { bool _r = _t->banClientInRegistry((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 51: { bool _r = _t->revokeClientCerts((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 52: { QMap<QString,TgClientRecord> _r = _t->readRegistry();
            if (_a[0]) *reinterpret_cast< QMap<QString,TgClientRecord>*>(_a[0]) = std::move(_r); }  break;
        case 53: _t->saveRegistry((*reinterpret_cast< std::add_pointer_t<QMap<QString,TgClientRecord>>>(_a[1]))); break;
        case 54: _t->saveClient((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<TgClientRecord&>>(_a[2]))); break;
        case 55: _t->logCertGenerationError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<CertGenerationResult>>(_a[2]))); break;
        case 56: _t->generateClientCertAsync((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QDate>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<TgMessage>>(_a[7]))); break;
        case 57: _t->handleNetworkError((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 58: _t->retryFailedRequest((*reinterpret_cast< std::add_pointer_t<QNetworkRequest>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[5]))); break;
        case 59: _t->onBtnStartStop(); break;
        case 60: _t->onBtnSaveSettings(); break;
        case 61: _t->onBtnTestToken(); break;
        case 62: _t->onBtnAddAdmin(); break;
        case 63: _t->onBtnRemoveAdmin(); break;
        case 64: _t->onBtnSendBroadcast(); break;
        case 65: _t->onBtnClearLog(); break;
        case 66: _t->onTokenVisibilityToggle(); break;
        case 67: _t->onPollTimeout(); break;
        case 68: _t->addLog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 69: _t->addLog((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 70: _t->updateStatusLabel(); break;
        case 71: _t->updateClientsTable(); break;
        case 72: _t->refreshAdminsList(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 57:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 58:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkRequest >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TelegramBotManager::*)(const QString & , const QString & );
            if (_t _q_method = &TelegramBotManager::logMessage; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TelegramBotManager::*)(bool );
            if (_t _q_method = &TelegramBotManager::statusChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (TelegramBotManager::*)(const QString & );
            if (_t _q_method = &TelegramBotManager::clientCreated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (TelegramBotManager::*)(const QString & );
            if (_t _q_method = &TelegramBotManager::clientRevoked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (TelegramBotManager::*)();
            if (_t _q_method = &TelegramBotManager::clientsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (TelegramBotManager::*)();
            if (_t _q_method = &TelegramBotManager::caGenerated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
}

const QMetaObject *TelegramBotManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TelegramBotManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TelegramBotManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TelegramBotManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 73)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 73;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 73)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 73;
    }
    return _id;
}

// SIGNAL 0
void TelegramBotManager::logMessage(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 2
void TelegramBotManager::statusChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void TelegramBotManager::clientCreated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void TelegramBotManager::clientRevoked(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void TelegramBotManager::clientsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void TelegramBotManager::caGenerated()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
