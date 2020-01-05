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

	ui->comboBox->addItem(QString("Без сортировки"));
	ui->comboBox->addItem(QString("Id"));
	ui->comboBox->addItem(QString("Id (обратная)"));
	ui->comboBox->addItem(QString("Время создания"));
	ui->comboBox->addItem(QString("Время создания (обратная)"));
	ui->comboBox->addItem(QString("Время изменения"));
	ui->comboBox->addItem(QString("Время изменения (обратная)"));
	ui->comboBox->addItem(QString("Кто менял"));
	ui->comboBox->addItem(QString("Кто менял (обратная)"));

	connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sortTypeComboboxIndexChanged(int)));

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

void MainWindow::sortTypeComboboxIndexChanged(int index)
{
	DatabaseManager::Instance().SortSelectionChanged(index);
}

//---------------------------------------------------------------

void MainWindow::closeEvent (QCloseEvent *)
{
	DatabaseManager::Instance().SaveDatabase();
}


//---------------------------------------------------------------

void MainWindow::on_pushButton_clicked()
{
//qDebug() << gTimer.elapsed() << " on_pushButton_clicked cp1";

	CHttpManager::Instance().Connect("mylogin", "mypassword", [](bool isSuccess){
		if (isSuccess) {
			Log("Connect succeded");
			DatabaseManager::Instance().LoadBaseAndRequestSync("TestDB"); // Загрузит базу если есть (если нет, создаст в памят пустую) и добавит запрос синхронизации в очередь сообщений на отсылку
		}
		else {
			Log("Connect failed");
		}
	});
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

//---------------------------------------------------------------

int MainWindow::GetSortTypeIndex()
{
	return ui->comboBox->currentIndex();

}


