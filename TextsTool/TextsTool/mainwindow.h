#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

//---------------------------------------------------------------

namespace Ui {
class MainWindow;
}

//---------------------------------------------------------------

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
	static MainWindow& Instance() { return *pthis; }
	explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
	void closeEvent (QCloseEvent *event);
	void SetModelForMainTable(QAbstractTableModel* model);
	QTreeWidget* getTreeWidget();
	int GetSortTypeIndex();
	std::vector<std::pair<QString, uint8_t>>& GetSortSelectors();

private slots:
	void on_pushButton_clicked();
	void update();
	void treeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void sortTypeComboboxIndexChanged(int index);
	void treeViewPrepareContextMenu( const QPoint & pos );
	void treeViewContextMenuCreateText();
	void treeViewContextMenuCreateFolder();

private:
    Ui::MainWindow *ui = nullptr;
	QTimer *_timer = nullptr;
	std::vector<std::pair<QString, uint8_t>> _sortSelectorItems;

	static MainWindow* pthis;
};

#endif // MAINWINDOW_H
