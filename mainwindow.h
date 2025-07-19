#pragma once

#include "fileinfo.h"

#include <QFutureWatcher>
#include <QWidget>

namespace Ui {
class MainWindow;
}

class QTreeWidgetItem;
class QProgressBar;
class FoundFilesModel;

class MainWindow : public QWidget
{
public:
	MainWindow(const QString &fileName, QWidget *parent = nullptr);
	~MainWindow();

private:
	std::unique_ptr<Ui::MainWindow> ui;
	QTreeWidgetItem *m_currentFileItem{nullptr};
	QFutureWatcher<FileInfo> m_watcher;
	QProgressBar *m_progressBar{nullptr};
	FoundFilesModel *m_foundFiles{nullptr};

protected:
	void closeEvent(QCloseEvent *event) override;
};
