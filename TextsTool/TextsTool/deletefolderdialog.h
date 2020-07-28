#ifndef DELETEFOLDERDIALOG_H
#define DELETEFOLDERDIALOG_H

#include <QDialog>

namespace Ui {
class DeleteFolderDialog;
}

class DeleteFolderDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DeleteFolderDialog(QWidget *parent = nullptr);
	~DeleteFolderDialog();

public slots:
	void okPressed();

private:
	Ui::DeleteFolderDialog *ui;
};

#endif // DELETEFOLDERDIALOG_H
