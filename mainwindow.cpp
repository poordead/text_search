#include "mainwindow.h"
#include "foundfilesmodel.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QFutureWatcher>
#include <QProgressBar>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

#include <minizip/unzip.h>
#include <zlib.h>

static void extractZip(QPromise<FileInfo> &promise,
					   const QString &zipPath,
					   const std::string &search_str)
{
	unzFile zipfile = unzOpen(zipPath.toLocal8Bit());
	if (!zipfile) {
		qWarning() << "Cannot open zip file";
		return;
	}

	unz_global_info global_info;
	if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK) {
		qWarning() << "Could not read file info";
		unzClose(zipfile);
		return;
	}

	promise.setProgressRange(0, global_info.number_entry);

	if (unzGoToFirstFile(zipfile) != UNZ_OK) {
		unzClose(zipfile);
		return;
	}

	int progressValue{0};

	do {
		promise.suspendIfRequested();
		if (promise.isCanceled())
			return;

		char filename[256];
		unz_file_info64 file_info;
		if (unzGetCurrentFileInfo64(
				zipfile, &file_info, filename, sizeof(filename), nullptr, 0, nullptr, 0)
			!= UNZ_OK) {
			break;
		}

		if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
			break;
		}

		promise.setProgressValueAndText(++progressValue, QString::fromLocal8Bit(filename));

		// Преобразуем DOS-дату в читаемый формат
		QDateTime lastModified{QDate{static_cast<int>(((file_info.dosDate >> 25) & 0x7f) + 1980),
									 static_cast<int>(((file_info.dosDate >> 21) & 0x0f) - 1),
									 static_cast<int>((file_info.dosDate >> 16) & 0x1f)},
							   QTime{static_cast<int>((file_info.dosDate >> 11) & 0x1f),
									 static_cast<int>((file_info.dosDate >> 5) & 0x3f),
									 static_cast<int>((file_info.dosDate & 0x1f) * 2)}};

		std::string buffer(1024, '\0');
		std::string prevBuffer;
		size_t pos = 0;

		int bytes_read;
		// Чтение файла поблочно
		while ((bytes_read = unzReadCurrentFile(zipfile, &buffer[0], buffer.size()))) {
			if (bytes_read < 0)
				break;
			// prevBuffer.append(buffer.substr(0, bytes_read));
			std::string data = prevBuffer + buffer.substr(0, bytes_read);
			size_t foundPos = data.find(search_str);

			if (foundPos != std::string::npos) {
				promise.addResult(
					{QString::fromLocal8Bit(filename), file_info.uncompressed_size, lastModified});
				break;
			}

			prevBuffer = buffer.substr(buffer.size() - search_str.size());
			pos += bytes_read;
		}

		unzCloseCurrentFile(zipfile);
	} while (unzGoToNextFile(zipfile) == UNZ_OK);

	unzClose(zipfile);
}

MainWindow::MainWindow(const QString &fileName, QWidget *parent)
	: QWidget(parent)
	, ui{std::make_unique<Ui::MainWindow>()}
{
	ui->setupUi(this);

	QTreeWidgetItem *analyzeItem = new QTreeWidgetItem(ui->summary_treeWidget, {tr("Анализ")});
	new QTreeWidgetItem(analyzeItem, {fileName});
	m_currentFileItem = new QTreeWidgetItem(analyzeItem);
	analyzeItem->setExpanded(true);

	auto progressItem = new QTreeWidgetItem(analyzeItem);
	ui->summary_treeWidget->setItemWidget(progressItem, 0, m_progressBar = new QProgressBar);

	QTreeWidgetItem *saveItem = new QTreeWidgetItem(ui->summary_treeWidget, {tr("Сохранение")});
	new QTreeWidgetItem(saveItem);

	ui->foundFiles->setModel(m_foundFiles = new FoundFilesModel{this});

	connect(&m_watcher,
			&QFutureWatcherBase::progressValueChanged,
			m_progressBar,
			&QProgressBar::setValue);
	connect(&m_watcher,
			&QFutureWatcherBase::progressRangeChanged,
			m_progressBar,
			&QProgressBar::setRange);
	connect(&m_watcher, &QFutureWatcherBase::progressTextChanged, this, [this](const QString &text) {
		m_currentFileItem->setText(0, text);
	});
	connect(&m_watcher,
			&QFutureWatcherBase::progressValueChanged,
			m_progressBar,
			&QProgressBar::setValue);

	connect(&m_watcher, &QFutureWatcherBase::resultReadyAt, this, [this](int resultIndex) {
		if (m_foundFiles->rowCount() < resultIndex + 1) {
			m_foundFiles->insertRow(m_foundFiles->rowCount());
		}
		m_foundFiles->setFileInfo(resultIndex, m_watcher.resultAt(resultIndex));
	});

	m_watcher.setFuture(QtConcurrent::run(extractZip, fileName, "secret"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	m_watcher.cancel();
}
