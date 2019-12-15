#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <string>
#include <QObject>
#include <QList>
#include <QString>
#include <QDebug>

#include "../SharedSrc/SerializationBuffer.h"
#include "../SharedSrc/DeserializationBuffer.h"
#include "CMessagesRepacker.h"
#include "Utils.h"
#include "DbSerializer.h"

Ui::MainWindow* debugGlobalUi = nullptr;
const static std::string databasePath = "D:/Dimka/HomeProjects/TextsTool/DatabaseClient/";
const static int KeyPerTextsNum = 100;  // На такое количество текстов создаётся один ключ для запроса RequestSync

//---------------------------------------------------------------

MainTableModel::MainTableModel(TextsDatabasePtr& dataBase, MessagesManager& messagesManager) :
	QAbstractTableModel(nullptr), _dataBase(dataBase), _messagesManager(messagesManager)
{
}

//---------------------------------------------------------------

int MainTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	if (!_dataBase || !_dataBase->isSynced) {
		return 0;
	}

	return _textsToShow.size();
}

//---------------------------------------------------------------

int MainTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	if (!_dataBase || !_dataBase->isSynced) {
		return 0;
	}

	return _columnsToShow.size();
}

//---------------------------------------------------------------

bool MainTableModel::getTextReferences(const QModelIndex &index, bool needCreateAttrIfNotFound, FoundTextRefs& result)
{
	result.text = _textsToShow[index.row()];
	result.attrInTable = &_dataBase->_attributeProps[_columnsToShow[index.column()]];
	if (result.attrInTable->type == AttributePropertyDataType::Id_t) {
		result.string = &result.text->id;
		return true;
	}
	if (result.attrInTable->type == AttributePropertyDataType::BaseText_t) {
		result.string = &result.text->baseText;
		return true;
	}
	for (auto& attribInText: result.text->attributes) {
		if (attribInText.id == result.attrInTable->id) {
			result.string = &attribInText.text;
			result.attrInText = &attribInText;
			return true;
		}
	}
	if (!needCreateAttrIfNotFound) {
		return false;
	}

	result.wasAttrInTextCreated = true;
	result.text->attributes.emplace_back();
	AttributeInText& attribInText = result.text->attributes.back();
	result.attrInText = &attribInText;
	attribInText.id = result.attrInTable->id;
	attribInText.type = result.attrInTable->type;
	result.string = &attribInText.text;

	return true;
}

//---------------------------------------------------------------

QVariant MainTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || !_dataBase || !_dataBase->isSynced || (role != Qt::DisplayRole &&  role != Qt::EditRole))	{
		return QVariant();
	}

	if(index.column() < 0 ||
			index.column() >= _columnsToShow.size() ||
			index.row() < 0 ||
			index.row() >= _textsToShow.size())	{
		qDebug() << "Warning: " << index.row() << ", " << index.column();
		return QVariant();
	}

	FoundTextRefs textRefs;
	bool isFound = const_cast<MainTableModel*>(this)->getTextReferences(index, false, textRefs);
	if (!isFound) {
		return QVariant();
	}
	return QString(textRefs.string->c_str());
}

//---------------------------------------------------------------

bool MainTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || !_dataBase || !_dataBase->isSynced || role != Qt::EditRole)	{
		return false;
	}

	if(index.column() < 0 ||
			index.column() >= _columnsToShow.size() ||
			index.row() < 0 ||
			index.row() >= _textsToShow.size())	{
		qDebug() << "Warning: " << index.row() << ", " << index.column();
		return false;
	}

	FoundTextRefs textRefs;
	bool isFound = getTextReferences(index, true, textRefs);
	if (!isFound) {
		return false;
	}

	*textRefs.string = value.toString().toUtf8().data();
	_messagesManager.SendMsgTextModified(textRefs);

	emit(dataChanged(index, index));
	return true;
}

//---------------------------------------------------------------

Qt::ItemFlags MainTableModel::flags(const QModelIndex &index) const
{
	if (!index.isValid() || !_dataBase || !_dataBase->isSynced)	{
		return Qt::ItemIsEnabled;
	}

	if(index.column() < 0 ||
			index.column() >= _columnsToShow.size() ||
			index.row() < 0 ||
			index.row() >= _textsToShow.size())	{
		qDebug() << "Warning: " << index.row() << ", " << index.column();
		return Qt::ItemIsEnabled;
	}

	return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

//---------------------------------------------------------------

QHash<int, QByteArray> MainTableModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[0] = "one";
	roles[1] = "two";
	roles[2] = "three";
	return roles;
}

//---------------------------------------------------------------

void MainTableModel::theDataChanged()
{
	//TODO
}

//---------------------------------------------------------------

QVariant MainTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal || section < 0 || section >= _columnsToShow.size())
		return QVariant();

	AttributeProperty& prop = _dataBase->_attributeProps[_columnsToShow[section]];
	return tr(prop.name.c_str());
}

//---------------------------------------------------------------

void MainTableModel::fillTextsToShowIndices()
{
	_textsToShow.clear();
	for (auto& folder: _dataBase->_folders) {
		for (auto& text: folder.texts) {
			_textsToShow.emplace_back(text.get());
		}
	}
}

//---------------------------------------------------------------

void MainTableModel::recalcColumnToShowData()
{
	_columnsToShow.clear();
	for (int i=0; i<_dataBase->_attributeProps.size(); ++i) {
		for (int i2=0; i2<_dataBase->_attributeProps.size(); ++i2) {
			auto& attrib = _dataBase->_attributeProps[i2];
			if (attrib.isVisible && attrib.visiblePosition == i) {
				_columnsToShow.push_back(i2);
			}
		}
	}
}


//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
	ui(new Ui::MainWindow),
	_messagesManager(this)
{
    Log("\n\n=== Start App ===========================================================");
	_timer = new QTimer(this);
	connect(_timer, SIGNAL(timeout()), this, SLOT(update()));
	_timer->start(50);

    ui->setupUi(this);
    debugGlobalUi = ui;

	_mainTableModel = std::make_unique<MainTableModel>(_dataBase, _messagesManager);
	ui->tableView->setModel(_mainTableModel.get());
}

//---------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}

//---------------------------------------------------------------

void MainWindow::on_pushButton_clicked()
{
//	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());

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
/*
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

*/
	//-------------------

	LoadBaseAndRequestSync("TestDB"); // Загрузит базу если есть (если нет, создаст в памят пустую) и добавит запрос синхронизации в очередь сообщений на отсылку

	_httpManager.Connect("mylogin", "mypassword");
}

//---------------------------------------------------------------

void MainWindow::ApplyDiffForSync(DeserializationBuffer& buf)
{
	std::string dbName;
	buf.GetString8(dbName);
	if (_dataBase && _dataBase->_dbName != dbName) {
		_dataBase->_dbSerializer->SaveDatabase();
		_dataBase.reset();
	}
	if (!_dataBase) {
		_dataBase = std::make_shared<TextsDatabase>(databasePath, dbName);
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

	_dataBase->isSynced = true;
	_dataBase->_dbSerializer->SaveDatabase();
}

//---------------------------------------------------------------

void MainWindow::LoadBaseAndRequestSync(const std::string& dbName)
{
	_dataBase = std::make_shared<TextsDatabase>(databasePath, dbName);

	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	auto& buf = *_msgsQueueOut.back();
	buf.PushUint8(EventType::RequestSync);
	buf.PushString8(_dataBase->_dbName);
	buf.PushUint32(_dataBase->_folders.size());
	for (auto& folder: _dataBase->_folders) {
		buf.PushUint32(folder.id);
		buf.PushUint32(folder.timestampModified);

		if (folder.texts.size() < KeyPerTextsNum) {
			buf.PushUint32(0);
			continue;
		}

		// Заполняем ключи текстов и ссылки на них для быстрой сортировки по ключам

		std::vector<std::vector<uint8_t>> textsKeys;       // Ключи серверных текстов текущей папки (для сортировки и разбиения на интевалы)

		textsKeys.resize(folder.texts.size());
		_textsKeysRefs.resize(folder.texts.size());

		for (int i=0; i<folder.texts.size(); ++i) {
			_textsKeysRefs[i] = i;
			MakeKey(folder.texts[i]->timestampModified, folder.texts[i]->id, textsKeys[i]); // Склеивает ts модификации текста и текстовый айдишник текста в "ключ"
		}

		// Сортируем ссылки на заполненные ключи текстов
		std::sort(_textsKeysRefs.begin(), _textsKeysRefs.end(), [&textsKeys](int el1, int el2) {
				auto& el1Ref = textsKeys[el1];
				auto& el2Ref = textsKeys[el2];
				return IfKeyALess(&el1Ref[0], el1Ref.size(), &el2Ref[0], el2Ref.size());
			});

		int selectedKeysNum = folder.texts.size() / KeyPerTextsNum;
		buf.PushUint32(selectedKeysNum);
		_intervals.resize(selectedKeysNum + 1);
		int offs = (folder.texts.size() - (selectedKeysNum - 1) * KeyPerTextsNum) / 2;
		// Добавляем в буфер ключи текстов и заполняем границы интервалов
		int v = 0;
		for (int i=0; i<selectedKeysNum; ++i) {
			int index = i*KeyPerTextsNum + offs;
			buf.PushVector8(textsKeys[_textsKeysRefs[index]]);
			_intervals[i].firstTextIdx = v;
			_intervals[i].textsNum = index - v;
			v = index;
		}
		_intervals[selectedKeysNum].firstTextIdx = v;
		_intervals[selectedKeysNum].textsNum = folder.texts.size() - v;

		// Расчёт и добавляем в буфер CRC64 интервалов
		for (auto& interval: _intervals) {
			uint64_t crc = 0;
			for (uint32_t i = interval.firstTextIdx; i < (int)interval.firstTextIdx + interval.textsNum; ++i) {
				crc = Utils::AddHash(crc, textsKeys[_textsKeysRefs[i]], i == interval.firstTextIdx);
			}
			buf.PushUint64(crc);
		}
	}
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
		ApplyDiffForSync(dbuf);
		_dataBase->LogDatabase();
		_mainTableModel->fillTextsToShowIndices();
		_mainTableModel->recalcColumnToShowData();
		ui->tableView->setModel(nullptr);
		ui->tableView->setModel(_mainTableModel.get());

//_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
//_msgsQueueOut.back()->PushUint8(EventType::ChangeBaseText);
//_msgsQueueOut.back()->PushString8("TextID1");
//_msgsQueueOut.back()->PushString16("NewBaseText2");
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

//---------------------------------------------------------------

void MessagesManager::SendMsgTextModified(const FoundTextRefs& textRefs)
{
	_mainWindow->_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	auto& buf = *_mainWindow->_msgsQueueOut.back();
	buf.PushUint8(EventType::ChangeAttributeInText);
	buf.PushString8(textRefs.text->id);
	buf.PushUint8(textRefs.attrInText->id);
	buf.PushUint8(textRefs.attrInText->type);
	buf.PushString16(textRefs.attrInText->text);
}
