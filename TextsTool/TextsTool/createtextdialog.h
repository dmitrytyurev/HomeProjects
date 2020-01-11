#ifndef CREATETEXTDIALOG_H
#define CREATETEXTDIALOG_H

#include <QDialog>

namespace Ui {
class CreateTextDialog;
}

class CreateTextDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CreateTextDialog(QWidget *parent = nullptr);
	~CreateTextDialog();

public slots:
	void okPressed();

private:
	Ui::CreateTextDialog *ui;
};

#endif // CREATETEXTDIALOG_H
