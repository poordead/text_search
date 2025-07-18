#include "foundfilesmodel.h"

FoundFilesModel::FoundFilesModel(QObject *parent)
	: QAbstractItemModel{parent}
{}

QModelIndex FoundFilesModel::index(int row, int column, const QModelIndex &parent) const
{
	return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
}

QModelIndex FoundFilesModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

int FoundFilesModel::rowCount(const QModelIndex &parent) const
{
	return m_data.size();
}

int FoundFilesModel::columnCount(const QModelIndex &parent) const
{
	return C_Count;
}

QVariant FoundFilesModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
		case C_Filename:
			return m_data.at(index.row()).fileName;
		case C_FileSize:
			return role == Qt::EditRole
					   ? QVariant{m_data.at(index.row()).fileSize}
					   : QVariant{QLocale().formattedDataSize(m_data.at(index.row()).fileSize)};
		case C_Timestamp:
			return m_data.at(index.row()).lastModified;
		}
	}
	return QVariant();
}

bool FoundFilesModel::insertRows(int row, int count, const QModelIndex &parent)
{
	beginInsertRows(QModelIndex(), row, row + count - 1);

	for (int i = 0; i < count; ++i)
		m_data.insert(row, {});

	endInsertRows();
	return true;
}

void FoundFilesModel::setFileInfo(int row, const FileInfo &fi)
{
	if (m_data.count() <= row)
		return;
	m_data[row] = fi;
	emit dataChanged(index(row, 0), index(row, C_Timestamp));
}

QVariant FoundFilesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section) {
		case C_Filename:
			return tr("Имя файла");
		case C_FileSize:
			return tr("Размер");
		case C_Timestamp:
			return tr("Последнее изменение");
		}
	}
	return QVariant();
}
