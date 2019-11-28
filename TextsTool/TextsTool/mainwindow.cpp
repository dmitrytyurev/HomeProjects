#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <string>

#include "SerializationBuffer.h"
#include "DeserializationBuffer.h"
#include "CMessagesRepacker.h"
#include "Utils.h"
#include "TextsBaseClasses.h"
#include "DbSerializer.h"

Ui::MainWindow* debugGlobalUi = nullptr;
const static std::string databasePath = "D:/Dimka/HomeProjects/";

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
	_timer = new QTimer(this);
	connect(_timer, SIGNAL(timeout()), this, SLOT(update()));
	_timer->start(50);

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

	_httpManager.Connect("mylogin", "mypassword");
}

//---------------------------------------------------------------

void MainWindow::ProcessSync(DeserializationBuffer& buf)
{
	std::string dbName;
	buf.GetString8(dbName);
	if (_dataBase && _dataBase->_dbName != dbName) {
		_dataBase.reset();
	}
	if (!_dataBase) {
		_dataBase = std::make_shared<TextsDatabase>();
		_dataBase->CreateDatabase(databasePath, dbName);
	}

	// Загружаем атрибуты заново с сервера
	_dataBase->_attributeProps.clear();
	int attribsNum = buf.GetUint32();
	for (int i=0; i<attribsNum; ++i) {
		_dataBase->_attributeProps.emplace_back();
		_dataBase->_attributeProps.back().LoadFullDump(buf);
	}

	// Удаляем каталоги, которых уже нет на сервере
	int foldersToDel = buf.GetUint32();
	for (int i=0; i<foldersToDel; ++i) {
		int folderToDelId = buf.GetUint32();
		auto& folders = _dataBase->_folders;
		folders.erase(std::remove_if(folders.begin(), folders.end(), [folderToDelId](const auto& el) { return el.id == folderToDelId; }), folders.end());
	}

	// Заполним таблицу для быстрой перекодировки id атрибута в его type
	std::vector<uint8_t> attributesIdToType;
	for (auto& attrib: _dataBase->_attributeProps) {
		if (attrib.id >= attributesIdToType.size()) {
			attributesIdToType.resize(attrib.id + 1);
		}
		attributesIdToType[attrib.id] = attrib.type;
	}

	// Добавляем каталоги с текстами, которых нет на клиенте
	int foldersToCreate = buf.GetUint32();
	for (int i=0; i<foldersToCreate; ++i) {
		_dataBase->_folders.emplace_back();
		_dataBase->_folders.back().LoadFullDump(buf, attributesIdToType);
	}

	// Патчим каталоги с текстами, которые есть на клиенте, но устаревшие
	int foldersToPatch = buf.GetUint32();
	for (int i=0; i<foldersToPatch; ++i) {
		int folderId = buf.GetUint32();
		auto& f = _dataBase->_folders;
		auto folderIt = std::find_if(std::begin(f), std::end(f), [folderId](const Folder& el) { return el.id == folderId; });
		if (folderIt == std::end(f)) {
			ExitMsg("folderFound == std::end(f)");
		}
		buf.GetString16(folderIt->name);
		folderIt->parentId = buf.GetUint32();
		folderIt->timestampModified = buf.GetUint32();
		uint8_t nextDataType = buf.GetUint8();
		if (nextDataType == FolderDataTypeForSyncMsg::AllTexts) {
			// Присланы все тексты каталога
			folderIt->texts.clear();
			int textsNum = buf.GetUint32();
			for (int i2 = 0; i2 < textsNum; ++i2) {
				folderIt->texts.emplace_back(std::make_shared<TextTranslated>());
				folderIt->texts.back()->LoadFullDump(buf, attributesIdToType);
			}
		}
		else {
			// Прислана часть текстов каталога (только для изменившихся интервалов)
			if (nextDataType != FolderDataTypeForSyncMsg::DiffTexts) {
				ExitMsg("nextDataType != FolderDataTypeForSyncMsg::DiffTexts");
			}
			int changedIntervalsNum = buf.GetUint32();
			for (int i2 = 0; i2 < changedIntervalsNum; ++i2) {
				int changedIntervalId = buf.GetUint32();
				// Обнулим указатели на тексты данного интервала (указатели из вектора удалим после обработки всех интервалов)
				int textsInIntervalOldNum = _intervals[changedIntervalId].textsNum;
				for (int i3 = 0; i3 < textsInIntervalOldNum; ++i3) {
					int index = i3 + _intervals[changedIntervalId].firstTextIdx;
					folderIt->texts[_textsKeysRefs[index]].reset();
				}
				// Добавим новые тексты интервала в папку
				int textsInIntervalNewNum = buf.GetUint32();
				for (int i3 = 0; i3 < textsInIntervalNewNum; ++i3) {
					folderIt->texts.emplace_back(std::make_shared<TextTranslated>());
					folderIt->texts.back()->LoadFullDump(buf, attributesIdToType);
				}
			}
			// Удалим из вектора указатели на тексты, которые были обнулены выше
			folderIt->texts.erase(std::remove_if(folderIt->texts.begin(), folderIt->texts.end(), [](const auto& el) { return el; }), folderIt->texts.end());
		}
	}

	_dataBase->_dbSerializer->SaveDatabase();
}


//---------------------------------------------------------------

void MainWindow::ProcessMessageFromServer(const std::vector<uint8_t>& buf)
{
	DeserializationBuffer dbuf(buf); // !!! Неоптимально! Этот буфер может быть сотню Мб! Подумать о работе по указателю без копирования данных
	uint8_t msgType = dbuf.GetUint8();
	switch(msgType) {
	case EventType::ReplySync:
	{
		Log("Msg: ReplySync");
		ProcessSync(dbuf);
	}
	break;
	default:
		Log("Unknown msgType");
	}
}

//---------------------------------------------------------------

void MainWindow::update()
{
	_httpManager.Update();
	Repacker::RepackPacketsInToMessages(_httpManager, _msgsQueueIn);
	Repacker::RepackMessagesOutToPackets(_msgsQueueOut, _httpManager);

	for (auto& msg: _msgsQueueIn) {
		Log("Message:  ");
		Utils::LogBuf(msg->_buffer);
		ProcessMessageFromServer(msg->_buffer);
	}
	_msgsQueueIn.resize(0);
}
