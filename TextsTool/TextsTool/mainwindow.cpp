#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <string>
#include <QObject>
#include <QList>
#include <QString>
#include <QDebug>
#include <QDropEvent>

#include "createtextdialog.h"
#include "createfolderdialog.h"
#include "deletefolderdialog.h"
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

	// --- Настройка treeWidget ---
	ui->treeWidget->setColumnCount(1);
	ui->treeWidget->setHeaderLabels(QStringList() << "Папки с текстами");
	connect(ui->treeWidget->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(treeSelectionChanged(const QItemSelection&,const QItemSelection&)));
	ui->treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
	ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &MainWindow::treeViewPrepareContextMenu);

	// --- Настройка tableView ---

	ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tableViewPrepareContextMenu(QPoint)));

	// --- Настройка comboBox ---
	std::vector<std::pair<QString, uint8_t>> sortSelectorItems = { {"Без сортировки", 255},
																   {"Id", AttributePropertyType::Id_t},
																   {"Id (обратная)", AttributePropertyType::Id_t},
																   {"Время создания", AttributePropertyType::CreationTimestamp_t},
																   {"Время создания (обратная)", AttributePropertyType::CreationTimestamp_t},
																   {"Время изменения", AttributePropertyType::ModificationTimestamp_t},
																   {"Время изменения (обратная)", AttributePropertyType::ModificationTimestamp_t},
																   {"Кто менял", AttributePropertyType::LoginOfLastModifier_t},
																   {"Кто менял (обратная)", AttributePropertyType::LoginOfLastModifier_t}
																 };
	_sortSelectorItems = sortSelectorItems;
	for (auto& selector: _sortSelectorItems) {
		ui->comboBox->addItem(selector.first);
	}
	connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(sortTypeComboboxIndexChanged(int)));

	// --- Настройка таймера ---
	_timer = new QTimer(this);
	connect(_timer, SIGNAL(timeout()), this, SLOT(update()));
	_timer->start(UpdateCallTimoutMs);
	gTimer.start();
}

//---------------------------------------------------------------

MainWindow::~MainWindow()
{
    delete ui;
}

//---------------------------------------------------------------

std::vector<std::pair<QString, uint8_t>>& MainWindow::GetSortSelectors()
{
	return _sortSelectorItems;
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

void MainWindow::treeViewPrepareContextMenu(const QPoint& pos)
{
	QTreeWidget *tree = ui->treeWidget;

//	QTreeWidgetItem *nd = tree->itemAt( pos );

//	qDebug()<<pos<<nd->text(0);

	QAction* actCreateText = new QAction(QIcon(":/Resource/warning32.ico"), tr("&Создать текст"), this);
	actCreateText->setStatusTip(tr("Создать новый текст в выбранной папке"));
	connect(actCreateText, SIGNAL(triggered()), this, SLOT(treeViewContextMenuCreateText()));

	QAction* actPasteTexts = new QAction(QIcon(":/Resource/warning32.ico"), tr("&Переместить тексты сюда"), this);
	actPasteTexts->setStatusTip(tr("Переместить вырезанные тексты в выбранную папку"));
	connect(actPasteTexts, SIGNAL(triggered()), this, SLOT(treeViewContextMenuPasteTexts()));

	QAction* actCreateFolder = new QAction(QIcon(":/Resource/warning32.ico"), tr("&Создать папку"), this);
	actCreateFolder->setStatusTip(tr("Создать новую папку в выбранной папке"));
	connect(actCreateFolder, SIGNAL(triggered()), this, SLOT(treeViewContextMenuCreateFolder()));

	QAction* actDeleteFolder = new QAction(QIcon(":/Resource/warning32.ico"), tr("&Удалить папку"), this);
	actDeleteFolder->setStatusTip(tr("Удалить папку и все тексты в ней"));
	connect(actDeleteFolder, SIGNAL(triggered()), this, SLOT(treeViewContextMenuDeleteFolder()));

	QMenu menu(this);
	menu.addAction(actCreateText);
	menu.addAction(actPasteTexts);
	menu.addAction(actCreateFolder);
	menu.addAction(actDeleteFolder);

//	QPoint pt(pos);
	menu.exec( tree->mapToGlobal(pos) );
}

//---------------------------------------------------------------

void MainWindow::treeViewContextMenuCreateText()
{
	CreateTextDialog* createTextDialog = new CreateTextDialog(this);
	createTextDialog->show();
}
//---------------------------------------------------------------

void MainWindow::treeViewContextMenuPasteTexts()
{
	DatabaseManager::Instance().OnPasteTextsFromGUI();
}

//---------------------------------------------------------------

void MainWindow::treeViewContextMenuCreateFolder()
{
	CreateFolderDialog* createFolderDialog = new CreateFolderDialog(this);
	createFolderDialog->show();
}

//---------------------------------------------------------------

void MainWindow::treeViewContextMenuDeleteFolder()
{
	DeleteFolderDialog* deleteFolderDialog = new DeleteFolderDialog(this);
	deleteFolderDialog->show();
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
	ui->tableView->setModel(model);
}

//---------------------------------------------------------------

void MainWindow::update()
{
	CHttpManager::Instance().Update(UpdateCallTimoutMs);
	DatabaseManager::Instance().Update(ui);
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

//---------------------------------------------------------------

void MainWindow::SetFocusToTableCellAndStartEdit(QModelIndex index)
{
	ui->tableView->setCurrentIndex(index);
	ui->tableView->edit(index);
}

//---------------------------------------------------------------

void MainWindow::tableViewPrepareContextMenu(const QPoint & pos)
{
	QAction* actDelTexts = new QAction(QIcon(":/Resource/warning32.ico"), tr("&Удалить тексты"), this);
	actDelTexts->setStatusTip(tr("Удалить выбранные тексты"));
	connect(actDelTexts, SIGNAL(triggered()), this, SLOT(tableViewContextMenuDeleteTexts()));

	QAction* actCutTexts = new QAction(QIcon(":/Resource/warning32.ico"), tr("&Вырезать тексты"), this);
	actCutTexts->setStatusTip(tr("Вырезать выбранные тексты (для последующего переноса в другую папку)"));
	connect(actCutTexts, SIGNAL(triggered()), this, SLOT(tableViewContextMenuCutTexts()));

	QMenu menu(this);
	menu.addAction(actDelTexts);
	menu.addAction(actCutTexts);

	menu.exec(ui->tableView->mapToGlobal(pos));
}

//---------------------------------------------------------------

void MainWindow::tableViewContextMenuDeleteTexts()
{
	QModelIndexList indexes = ui->tableView->selectionModel()->selection().indexes();
	for (int i = 0; i < indexes.count(); ++i)
	{
		QModelIndex index = indexes.at(i);
		DatabaseManager::Instance().OnTextDeletedFromGUI(index.row());
	}
}

//---------------------------------------------------------------

void MainWindow::tableViewContextMenuCutTexts()
{
	QModelIndexList indexes = ui->tableView->selectionModel()->selection().indexes();
	std::vector<int> textsIndices;
	for (int i = 0; i < indexes.count(); ++i)
	{
		QModelIndex index = indexes.at(i);
		textsIndices.push_back(index.row());
	}
	DatabaseManager::Instance().OnTextsCutFromGUI(textsIndices);
}

//---------------------------------------------------------------

void QTreeWidgetMy::dragMoveEvent(QDragMoveEvent* event)
{
	itemStartDragFrom = this->currentItem();
}

//---------------------------------------------------------------

void QTreeWidgetMy::dropEvent(QDropEvent *event) 
{ 
	QModelIndex index = indexAt(event->pos());
	QTreeWidgetItem* item = itemFromIndex(index);

	Log("dropEvent! from: " + std::to_string(uint32_t(itemStartDragFrom)) + " to: " + std::to_string(uint32_t(item)));
	DatabaseManager::Instance().OnFolderDraggedOntoFolder(itemStartDragFrom, item);
}


