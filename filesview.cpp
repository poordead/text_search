#include "filesview.h"
#include "foundfilesmodel.h"

#include <QDir>
#include <QHeaderView>
#include <QSortFilterProxyModel>

class FileSizeDelegate : public QStyledItemDelegate
{
public:
	explicit FileSizeDelegate(QObject *parent = nullptr)
		: QStyledItemDelegate{parent}
	{}

private:
	QString displayText(const QVariant &value, const QLocale &locale) const override
	{
		return locale.formattedDataSize(value.toULongLong());
	}
};
class FileNameDelegate : public QStyledItemDelegate
{
public:
	explicit FileNameDelegate(const QString &fileName, QObject *parent = nullptr)
		: QStyledItemDelegate{parent}
		, m_fileName(fileName)
	{}

private:
	QString displayText(const QVariant &value, const QLocale &locale) const override
	{
		return QDir(m_fileName).filePath(value.toString());
	}

	const QString m_fileName;
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
	horizontalHeader()
		->setSectionResizeMode(FoundFilesModel::C_Filename, QHeaderView::QHeaderView::Stretch);
}

void FilesView::setFileName(const QString &fileName)
{
	setItemDelegateForColumn(FoundFilesModel::C_Filename, new FileNameDelegate{fileName, this});
}
