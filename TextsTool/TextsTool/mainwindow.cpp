#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <string>

#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "../SharedSrc/Shared.h"
#include "CMessagesRepacker.h"
#include "Utils.h"

Ui::MainWindow* debugGlobalUi = nullptr;

//---------------------------------------------------------------

void MakeKey(uint32_t tsModified, const std::string& textId, std::vector<uint8_t>& result)
{
	result.resize(sizeof(uint32_t) + textId.size());
	uint8_t* p = &result[0];

	*(reinterpret_cast<uint32_t*>(p)) = tsModified;
	memcpy(p + sizeof(uint32_t), textId.c_str(), textId.size());
}

//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    Log("\n\n=== Start App ===========================================================");
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(50);

    ui->setupUi(this);
    debugGlobalUi = ui;
}

//---------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}

//---------------------------------------------------------------

void MainWindow::on_pushButton_clicked()
{
	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());

/*
	_msgsQueueOut.back()->PushUint8(EventType::RequestSync);
	_msgsQueueOut.back()->PushString8("TestDB");
	_msgsQueueOut.back()->PushUint32(2); // Число папок


	_msgsQueueOut.back()->PushUint32(55443); // id папки
	_msgsQueueOut.back()->PushUint32(50000); // TS изменения папки на клиенте
	_msgsQueueOut.back()->PushUint32(0); // Количество отобранных ключей


	_msgsQueueOut.back()->PushUint32(11111); // id папки
	_msgsQueueOut.back()->PushUint32(54321); // TS изменения папки на клиенте
	_msgsQueueOut.back()->PushUint32(0); // Количество отобранных ключей
*/

	_msgsQueueOut.back()->PushUint8(EventType::RequestSync);
	_msgsQueueOut.back()->PushString8("TestDB");
	_msgsQueueOut.back()->PushUint32(1); // Число папок


	_msgsQueueOut.back()->PushUint32(55443322); // id папки
	_msgsQueueOut.back()->PushUint32(50000); // TS изменения папки на клиенте
	_msgsQueueOut.back()->PushUint32(3); // Количество отобранных ключей

	struct Text
	{
		std::string id;
		uint32_t ts;
		std::vector<uint8_t> key;
	};

	std::vector<Text> texts = {{"TextID1", 101}, {"TextID2", 102}, {"TextID3", 103}, {"TextID4", 104}, {"TextID5", 105}, {"TextID6", 106}, {"TextID7", 107}, {"TextID8", 108}, {"TextID9", 109}, {"TextID10", 110}};

	for (auto& text: texts) {
		MakeKey(text.ts, text.id, text.key);
	}

	// Ключи разбиения текстов
	_msgsQueueOut.back()->PushUint8(texts[2].key.size());
	_msgsQueueOut.back()->PushBytes(texts[2].key.data(), texts[2].key.size());

	_msgsQueueOut.back()->PushUint8(texts[4].key.size());
	_msgsQueueOut.back()->PushBytes(texts[4].key.data(), texts[4].key.size());

	_msgsQueueOut.back()->PushUint8(texts[7].key.size());
	_msgsQueueOut.back()->PushBytes(texts[7].key.data(), texts[7].key.size());

//Log("Key:");
//Utils::LogBuf(texts[1].key);
	// Инфа об интервалах (на 1 больше числа ключей)
	_msgsQueueOut.back()->PushUint32(2); // Число текстов в интервале
	uint64_t hash = 0;
	hash = Utils::AddHash(hash, texts[0].key, true);
	hash = Utils::AddHash(hash, texts[1].key, false);
	_msgsQueueOut.back()->PushBytes(&hash, sizeof(uint64_t));

	_msgsQueueOut.back()->PushUint32(2); // Число текстов в интервале
	hash = Utils::AddHash(hash, texts[2].key, true);
	hash = Utils::AddHash(hash, texts[3].key, false);
	_msgsQueueOut.back()->PushBytes(&hash, sizeof(uint64_t));

	_msgsQueueOut.back()->PushUint32(3); // Число текстов в интервале
	hash = Utils::AddHash(hash, texts[4].key, true);
	hash = Utils::AddHash(hash, texts[5].key, false);
	hash = Utils::AddHash(hash, texts[6].key, false);
	_msgsQueueOut.back()->PushBytes(&hash, sizeof(uint64_t));

	_msgsQueueOut.back()->PushUint32(3); // Число текстов в интервале
	hash = Utils::AddHash(hash, texts[7].key, true);
	hash = Utils::AddHash(hash, texts[8].key, false);
	hash = Utils::AddHash(hash, texts[9].key, false);
	_msgsQueueOut.back()->PushBytes(&hash, sizeof(uint64_t));

	//-------------------

	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	_msgsQueueOut.back()->PushUint8(111); // Изменить основной текст
	_msgsQueueOut.back()->PushString8("TextID1");
	_msgsQueueOut.back()->PushString16("NewBaseText1");




	//-------------------

    httpManager.Connect("mylogin", "mypassword");
}

//---------------------------------------------------------------

void MainWindow::update()
{
	httpManager.Update();
	Repacker::RepackPacketsInToMessages(httpManager, _msgsQueueIn);
	Repacker::RepackMessagesOutToPackets(_msgsQueueOut, httpManager);

	for (auto& msg: _msgsQueueIn) {
		Log("Message:  ");
		Utils::LogBuf(msg->_buffer);
	}
	_msgsQueueIn.resize(0);
}
