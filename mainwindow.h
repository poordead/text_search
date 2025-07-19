#pragma once

#include "fileinfo.h"

#include <QFutureWatcher>
#include <QWidget>

namespace Ui {
class MainWindow;
}

class QTreeWidgetItem;
class QProgressBar;
class QPushButton;
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
	QFutureWatcher<void> m_saveWatcher;
	QProgressBar *m_progressBar{nullptr};
	QProgressBar *m_saveProgressBar{nullptr};
	QPushButton *m_saveButton{nullptr};
	FoundFilesModel *m_foundFiles{nullptr};
	const QString m_fileName;

private:
	void closeEvent(QCloseEvent *event) override;

	void saveZip();
};
