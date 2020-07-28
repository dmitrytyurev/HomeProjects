#ifndef CREATEFOLDERDIALOG_H
#define CREATEFOLDERDIALOG_H

#include <QDialog>

namespace Ui {
class CreateFolderDialog;
}

class CreateFolderDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CreateFolderDialog(QWidget *parent = nullptr);
	~CreateFolderDialog();

public slots:
	void okPressed();

private:
	Ui::CreateFolderDialog *ui;
};

#endif // CREATEFOLDERDIALOG_H
