#pragma once

#include "fileinfo.h"

#include <QAbstractItemModel>
#include <QList>

class FoundFilesModel : public QAbstractItemModel
{
public:
	enum Columns { C_Filename, C_FileSize, C_Timestamp, C_Count };
	explicit FoundFilesModel(QObject *parent = nullptr);

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	bool insertRows(int row, int count, const QModelIndex &parent) override;
	void setFileInfo(int row, const FileInfo &fi);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

private:
	QModelIndex parent(const QModelIndex &child) const override;
	QList<std::tuple<Qt::CheckState, FileInfo>> m_data;
};
