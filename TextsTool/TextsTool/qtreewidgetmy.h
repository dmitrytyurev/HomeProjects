#include "Utils.h"

class QTreeWidgetMy: public QTreeWidget
{
public:
	QTreeWidgetMy(QWidget* widget) : QTreeWidget(widget)  {}
	void dropEvent(QDropEvent *event) { Log("dropEvent!!!"); }

};