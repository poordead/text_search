#include "minizip_wrapper.h"
#include "fileinfo.h"

#include <QPromise>
#include <queue>
#include <thread>
#include <unzip.h>
#include <zip.h>

void findTextInZip(QPromise<FileInfo> &promise,
				   const QString &zipPath,
				   const std::string &search_str)
{
    unzFile zipfile = unzOpen64(zipPath.toUtf8());
    if (!zipfile) {
        promise.setException(std::make_exception_ptr("Cannot open zip file"));
        return;
    }

    unz_global_info global_info;
    if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK) {
		unzClose(zipfile);
		promise.setException(std::make_exception_ptr("Could not read file info"));
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
		// The ZIP format specification added explicit UTF-8 support in version 6.3 (2006)
		// 	Many modern zip tools always use UTF-8 regardless of the flag
		// 	If we need to support legacy archives, might need CP437 conversion

		// Safe buffer sizes when working with minizip
		constexpr uLong MAX_FILENAME_LENGTH = 1024; // More than enough for most cases
		char fileName[MAX_FILENAME_LENGTH];
		unz_file_info64 file_info;
		if (unzGetCurrentFileInfo64(
				zipfile, &file_info, fileName, sizeof(fileName), nullptr, 0, nullptr, 0)
			!= UNZ_OK) {
			break;
		}

		if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
			break;
		}

		promise.setProgressValueAndText(++progressValue, QString::fromUtf8(fileName));

		const QDateTime lastModified{QDate{static_cast<int>(((file_info.dosDate >> 25) & 0x7f)
															+ 1980),
										   static_cast<int>(((file_info.dosDate >> 21) & 0x0f) - 1),
										   static_cast<int>((file_info.dosDate >> 16) & 0x1f)},
									 QTime{static_cast<int>((file_info.dosDate >> 11) & 0x1f),
										   static_cast<int>((file_info.dosDate >> 5) & 0x3f),
										   static_cast<int>((file_info.dosDate & 0x1f) * 2)}};

		constexpr size_t chunk_size = 1 << 20; // 1MB
		std::string buffer(chunk_size, '\0');
		std::string prevBuffer;

		int bytes_read;
		while ((bytes_read = unzReadCurrentFile(zipfile, &buffer[0], chunk_size))) {
			if (bytes_read < 0)
				break;
			prevBuffer.append(buffer.substr(0, bytes_read));

			if (const auto foundPos{prevBuffer.find(search_str)}; foundPos != std::string::npos) {
                unz64_file_pos file_pos;
                if (unzGetFilePos64(zipfile, &file_pos) != UNZ_OK) {
                    promise.setException(std::make_exception_ptr("Failed to get file position."));
                    break;
                }
                promise.addResult({QString::fromUtf8(fileName),
                                   file_info.uncompressed_size,
                                   lastModified,
                                   file_pos});
                break;
            }

            prevBuffer = buffer.substr(buffer.size() - search_str.size());
        }

        unzCloseCurrentFile(zipfile);
    } while (unzGoToNextFile(zipfile) == UNZ_OK);

    unzClose(zipfile);
}

template<typename T>
class ThreadSafeQueue
{
public:
    void push(const T &value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_queue.push(value);
        lock.unlock();
        m_cond.notify_one();
    }

    void complete()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_completed = true;
        m_cond.notify_all();
    }

    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return !m_queue.empty() || m_completed; });

        if (m_queue.empty() && m_completed) {
            return std::nullopt;
        }
        T value = m_queue.front();
        m_queue.pop();
        return value;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

private:
    std::queue<T> m_queue;
    bool m_completed = false;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
};

void zipSelectedFiles(QPromise<void> &promise,
                      const QString &zipPath,
                      const QString &newZipPath,
                      const QList<FileInfo> &files)
{
    QElapsedTimer et;
    et.start();
    unzFile zipfile = unzOpen64(zipPath.toUtf8());
    if (!zipfile) {
        promise.setException(std::make_exception_ptr("Cannot open zip file"));
        return;
    }

    promise.setProgressRange(0, files.size());

    zipFile zf = zipOpen64(newZipPath.toUtf8(), APPEND_STATUS_CREATE);

    if (!zf) {
        promise.setException(std::make_exception_ptr("Error creating zip file"));
		return;
    }
    int progressValue{0};

    for (const auto &fileInfo : files) {
        promise.suspendIfRequested();
        if (promise.isCanceled())
            return;
        const auto fileName{fileInfo.fileName};

        promise.setProgressValueAndText(++progressValue, fileName);

        if (unzGoToFilePos64(zipfile, &fileInfo.filePos) != UNZ_OK) {
            qWarning() << "cannot go to file pos" << fileName;
            continue;
        }

        if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
            qWarning() << "cannot open" << fileName;
            continue;
        }

        zip_fileinfo zi = {0};
        if (ZIP_OK
            != zipOpenNewFileInZip64(zf,
                                     fileName.toUtf8(),
                                     &zi,
                                     nullptr,
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     Z_DEFLATED,
                                     Z_DEFAULT_COMPRESSION,
                                     0)) {
            qWarning() << "Failed to add file to zip: " << fileName;
            continue;
        }

        ThreadSafeQueue<QByteArray> queue;

        std::thread zip_reader_thread([&queue, &zipfile] {
            constexpr qsizetype chunk_size = 1 << 20; // 1MB
            QByteArray buffer(chunk_size, Qt::Uninitialized);
            int bytes_read;
            while ((bytes_read = unzReadCurrentFile(zipfile, buffer.data(), chunk_size))) {
                if (bytes_read < 0)
                    break;
                queue.push(buffer.first(bytes_read));
            }
            queue.complete();
            unzCloseCurrentFile(zipfile);
        });

        std::thread zip_write_thread([&queue, zf, &fileName] {
            while (const auto value = queue.pop()) {
                if (ZIP_OK != zipWriteInFileInZip(zf, value->data(), value->size())) {
                    qWarning() << "Failed to write file chunk to zip: " << fileName;
                    break;
                }
            }
            zipCloseFileInZip(zf);
        });

        zip_reader_thread.join();
        zip_write_thread.join();
    }

    zipClose(zf, nullptr);
    unzClose(zipfile);
    qDebug() << et.elapsed();
}
