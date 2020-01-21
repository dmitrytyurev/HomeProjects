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

namespace Ui {
	class MainWindow;
}
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
	std::string localString;
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
	void OnTextCreatedFromGUI(const std::string& textIdToCreate);
	void OnTextDeletedFromGUI(int textIndex);
	void OnFolderCreatedFromGUI(const std::string& folderNameToCreate);
	void OnFolderDeletedFromGUI();
	void OnFolderDraggedOntoFolder(QTreeWidgetItem* itemFrom, QTreeWidgetItem* itemTo);
	void ProcessMessageFromServer(const std::vector<uint8_t>& buf);
	void SaveDatabase();
	void Update(Ui::MainWindow* ui);
	void TreeSelectionChanged();
	void SortSelectionChanged(int index);
	uint32_t GetSelectedFolder();

private:
	void ApplyDiffForSync(DeserializationBuffer& buf);
	std::string ModifyDbChangeBaseText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifie, std::vector<AttributeProperty*>* affectedAttributes);
	std::string ModifyDbChangeAttributeInText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifier, std::vector<AttributeProperty*>* affectedAttributes);
	std::string ModifyDbCreateText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifie);
	std::string ModifyDbDeleteText(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifie);
	void ModifyDbCreateFolder(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifie);
	void ModifyDbDeleteFolder(DeserializationBuffer& dbuf, uint32_t ts, const std::string& loginOfModifie);
	void SendMsgChangeBaseText(const FoundTextRefs& textRefs);
	void SendMsgChangeTextAttrib(const FoundTextRefs& textRefs);
	void SendMsgCreateNewText(const std::string& textId);
	void SendMsgDeleteText(int textIndex);
	void SendMsgCreateNewFolder(const std::string& textId, uint32_t parentFolderId);
	void SendMsgDeleteFolder();
	void SendMsgFolderChangeParent(uint32_t folderId, uint32_t newParentFolderId);
	void AdjustFolderView();
	void AdjustFolderViewRec(uint32_t parentId, QTreeWidgetItem *parentTreeItem);
	void ClearFolderViewRec(uint32_t parentId);
	Folder* FolderByTextId(const std::string& textId);

	static DatabaseManager* pthis;
	std::vector<SerializationBufferPtr>   _msgsQueueOut; // Очередь сообщений, которые нужно отослать на сервер
	std::vector<DeserializationBufferPtr> _msgsQueueIn;  // Очередь пришедших от сервера сообщений
	TextsDatabasePtr _dataBase;
	MainTableModelPtr _mainTableModel;  // Модель обеспечивающая доступ к данным главной таблицы текстов
	std::vector<int> _textsKeysRefs;  // Указатели на ключи текстов для быстрой сортировки
	std::vector<TextsInterval> _intervals;
};


