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
#include "DatabaseManager.h"
#include <QElapsedTimer>

const static int UpdateCallTimoutMs = 50; // Через сколько миллисекунд вызывается Update


QElapsedTimer gTimer;
Ui::MainWindow* debugGlobalUi = nullptr;
MainWindow* MainWindow::pthis = nullptr;

//---------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	pthis = this;
    Log("\n\n=== Start App ===========================================================");
    ui->setupUi(this);
    debugGlobalUi = ui;

	DatabaseManager::Init();
	CHttpManager::Init();
	_timer = new QTimer(this);
	connect(_timer, SIGNAL(timeout()), this, SLOT(update()));
	_timer->start(UpdateCallTimoutMs);

	ui->treeWidget->setColumnCount(1);
	ui->treeWidget->setHeaderLabels(QStringList() << "Папки с текстами");

	connect(ui->treeWidget->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(treeSelectionChanged(const QItemSelection&,const QItemSelection&)));
	gTimer.start();
}

//---------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}

//---------------------------------------------------------------

void MainWindow::treeSelectionChanged(const QItemSelection&, const QItemSelection&)
{
	DatabaseManager::Instance().TreeSelectionChanged();
}
//---------------------------------------------------------------

void MainWindow::closeEvent (QCloseEvent *)
{
	DatabaseManager::Instance().SaveDatabase();
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
qDebug() << gTimer.elapsed() << " on_pushButton_clicked cp1";
	DatabaseManager::Instance().LoadBaseAndRequestSync("TestDB"); // Загрузит базу если есть (если нет, создаст в памят пустую) и добавит запрос синхронизации в очередь сообщений на отсылку
qDebug() << gTimer.elapsed() << " on_pushButton_clicked cp2";
	CHttpManager::Instance().Connect("mylogin", "mypassword");
qDebug() << gTimer.elapsed() << " on_pushButton_clicked cp3";
}

//---------------------------------------------------------------
void MainWindow::SetModelForMainTable(QAbstractTableModel* model)
{
//	ui->tableView->setModel(nullptr);
	ui->tableView->setModel(model);
}

//---------------------------------------------------------------

void MainWindow::update()
{
	CHttpManager::Instance().Update(UpdateCallTimoutMs);
	DatabaseManager::Instance().Update();
}

//---------------------------------------------------------------

QTreeWidget* MainWindow::getTreeWidget()
{
	return ui->treeWidget;
}

