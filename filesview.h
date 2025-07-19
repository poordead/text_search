#pragma once

#include <QStyledItemDelegate>
#include <QTableView>

class FilesView : public QTableView
{
public:
	explicit FilesView(QWidget *parent = nullptr);

	void setModel(QAbstractItemModel *model) override;
	void setFileName(const QString &fileName);
};
