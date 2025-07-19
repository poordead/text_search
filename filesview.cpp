#include "filesview.h"
#include "foundfilesmodel.h"

#include <QSortFilterProxyModel>
#include <QtWidgets/qheaderview.h>

class FileSizeDelegate : public QStyledItemDelegate
{
public:
	explicit FileSizeDelegate(QObject *parent = nullptr)
		: QStyledItemDelegate{parent}
	{}

public:
	QString displayText(const QVariant &value, const QLocale &locale) const override
	{
		return locale.formattedDataSize(value.toULongLong());
	}
};

FilesView::FilesView(QWidget *parent)
	: QTableView(parent)
{
	setItemDelegateForColumn(FoundFilesModel::C_FileSize, new FileSizeDelegate{this});
}

void FilesView::setModel(QAbstractItemModel *model)
{
	auto sortModel = new QSortFilterProxyModel{model};
	sortModel->setSourceModel(model);

	QTableView::setModel(sortModel);
	horizontalHeader()->resizeSection(FoundFilesModel::C_Timestamp, 150);
	horizontalHeader()
		->setSectionResizeMode(FoundFilesModel::C_Filename, QHeaderView::QHeaderView::Stretch);
}
