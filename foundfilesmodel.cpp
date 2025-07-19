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
			return std::get<FileInfo>(m_data.at(index.row())).fileName;
		case C_FileSize:
			return std::get<FileInfo>(m_data.at(index.row())).fileSize;
		case C_Timestamp:
			return std::get<FileInfo>(m_data.at(index.row())).lastModified;
		}
	} else if (role == Qt::CheckStateRole) {
		switch (index.column()) {
		case C_Filename:
			return std::get<Qt::CheckState>(m_data.at(index.row()));
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
	m_data[row] = std::make_tuple(Qt::Unchecked, fi);
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
	return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags FoundFilesModel::flags(const QModelIndex &index) const
{
	auto ret{QAbstractItemModel::flags(index)};
	if (index.column() == C_Filename)
		ret |= Qt::ItemIsUserCheckable;

	return ret;
}

bool FoundFilesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::CheckStateRole && index.column() == C_Filename) {
		std::get<Qt::CheckState>(m_data[index.row()]) = value.value<Qt::CheckState>();
		emit dataChanged(index, index);
		return true;
	}

	return QAbstractItemModel::setData(index, value, role);
}
