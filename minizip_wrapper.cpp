#include "minizip_wrapper.h"

#include "fileinfo.h"

#include <QDir>
#include <QtCore/qpromise.h>
#include <unzip.h>

void findTextInZip(QPromise<FileInfo> &promise,
				   const QString &zipPath,
				   const std::string &search_str)
{
	unzFile zipfile = unzOpen64(
#ifdef Q_OS_WINDOWS
		reinterpret_cast<const wchar_t *>(zipPath.utf16())
#else
		zipPath.toUtf8()
#endif
	);
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

		promise.setProgressValueAndText(++progressValue, QString::fromUtf8(filename));

		// Преобразуем DOS-дату в читаемый формат
		const QDateTime lastModified{QDate{static_cast<int>(((file_info.dosDate >> 25) & 0x7f)
															+ 1980),
										   static_cast<int>(((file_info.dosDate >> 21) & 0x0f) - 1),
										   static_cast<int>((file_info.dosDate >> 16) & 0x1f)},
									 QTime{static_cast<int>((file_info.dosDate >> 11) & 0x1f),
										   static_cast<int>((file_info.dosDate >> 5) & 0x3f),
										   static_cast<int>((file_info.dosDate & 0x1f) * 2)}};

		std::string buffer(1024, '\0');
		std::string prevBuffer;

		int bytes_read;
		while ((bytes_read = unzReadCurrentFile(zipfile, &buffer[0], buffer.size()))) {
			if (bytes_read < 0)
				break;
			prevBuffer.append(buffer.substr(0, bytes_read));

			if (const auto foundPos{prevBuffer.find(search_str)}; foundPos != std::string::npos) {
				promise.addResult({QDir(zipPath).filePath(QString::fromUtf8(filename)),
								   file_info.uncompressed_size,
								   lastModified});
				break;
			}

			prevBuffer = buffer.substr(buffer.size() - search_str.size());
		}

		unzCloseCurrentFile(zipfile);
	} while (unzGoToNextFile(zipfile) == UNZ_OK);

	unzClose(zipfile);
}
