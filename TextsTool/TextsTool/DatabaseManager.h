#pragma once

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QTextCodec>
#include <QAbstractTableModel>
#include <QTreeWidget>

#include "../SharedSrc/Shared.h"
#include "../SharedSrc/DeserializationBuffer.h"
#include "../SharedSrc/SerializationBuffer.h"
#include "TextsBaseClasses.h"
#include "CHttpManager.h"
#include "MainTableModel.h"

class TextsDatabase;
class MainWindow;
using TextsDatabasePtr = std::shared_ptr<TextsDatabase>;


//---------------------------------------------------------------
struct FoundTextRefs
{
	TextTranslated* text = nullptr;
	AttributeInText* attrInText = nullptr;
	AttributeProperty* attrInTable = nullptr;
	std::string* string = nullptr;
	bool wasAttrInTextCreated = false;
};

//---------------------------------------------------------------

class DatabaseManager
{
public:
	DatabaseManager ();
	static void Init();
	static void Deinit();
	static DatabaseManager& Instance();
	void LoadBaseAndRequestSync(const std::string& dbName);
	void OnTextModifiedFromGUI(const FoundTextRefs& textRefs);
	void ProcessMessageFromServer(const std::vector<uint8_t>& buf);
	void SaveDatabase();
	void Update();

private:
	void ApplyDiffForSync(DeserializationBuffer& buf);
	std::string ModifyDbChangeBaseText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifie);
	std::pair<std::string, int> ModifyDbChangeAttributeInText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifier);
	void SendMsgChangeBaseText(const FoundTextRefs& textRefs);
	void SendMsgChangeTextAttrib(const FoundTextRefs& textRefs);
	void ResetTextAndFolderTimestamps(const FoundTextRefs& textRefs);
	void AdjustFolderView(uint32_t parentId, QTreeWidgetItem *parentTreeItem);

	static DatabaseManager* pthis;
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать на сервер
	std::vector<DeserializationBufferPtr> _msgsQueueIn;  // Очередь пришедших от сервера сообщений
	TextsDatabasePtr _dataBase;
	MainTableModelPtr _mainTableModel;  // Модель обеспечивающая доступ к данным главной таблицы текстов
	std::vector<int> _textsKeysRefs;  // Указатели на ключи текстов для быстрой сортировки
	std::vector<TextsInterval> _intervals;
};


