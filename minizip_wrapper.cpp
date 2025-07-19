#include "minizip_wrapper.h"
#include "fileinfo.h"

#include <QPromise>
#include <unzip.h>
#include <zip.h>

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
				promise.addResult(
					{QString::fromUtf8(fileName), file_info.uncompressed_size, lastModified});
				break;
			}

			prevBuffer = buffer.substr(buffer.size() - search_str.size());
		}

		unzCloseCurrentFile(zipfile);
	} while (unzGoToNextFile(zipfile) == UNZ_OK);

	unzClose(zipfile);
}

void zipSelectedFiles(QPromise<void> &promise,
					  const QString &zipPath,
					  const QString &newZipPath,
					  const std::vector<std::string> &files)
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

	promise.setProgressRange(0, files.size());

	zipFile zf = zipOpen64(
#ifdef Q_OS_WINDOWS
		reinterpret_cast<const wchar_t *>(newZipPath.utf16())
#else
		newZipPath.toUtf8()
#endif
			,
		APPEND_STATUS_CREATE);

	if (!zf) {
		promise.setException(std::make_exception_ptr("Error creating zip file"));
		return;
	}
	int progressValue{0};

	for (const auto &fileName : files) {
		promise.suspendIfRequested();
		if (promise.isCanceled())
			return;

		promise.setProgressValueAndText(++progressValue, QString::fromUtf8(fileName));

		if (unzLocateFile(zipfile, fileName.c_str(), 1) != UNZ_OK) {
			qWarning() << "cannot locate" << fileName.c_str();
			continue;
		}

		if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
			qWarning() << "cannot open" << fileName.c_str();
			continue;
		}

		zip_fileinfo zi = {0};
		if (ZIP_OK
			!= zipOpenNewFileInZip64(zf,
									 fileName.c_str(),
									 &zi,
									 nullptr,
									 0,
									 nullptr,
									 0,
									 nullptr,
									 Z_DEFLATED,
									 Z_DEFAULT_COMPRESSION,
									 0)) {
			qWarning() << "Failed to add file to zip: " << fileName.c_str();
			continue;
		}

		constexpr size_t chunk_size = 1 << 20; // 1MB
		std::vector<char> buffer(chunk_size);
		int bytes_read;
		while ((bytes_read = unzReadCurrentFile(zipfile, &buffer[0], chunk_size))) {
			if (bytes_read < 0)
				break;

			if (ZIP_OK != zipWriteInFileInZip(zf, buffer.data(), bytes_read)) {
				zipCloseFileInZip(zf);
				qWarning() << "Failed to write file chunk to zip: " << fileName.c_str();
				break;
			}
		}

		zipCloseFileInZip(zf);
		unzCloseCurrentFile(zipfile);
	}

	zipClose(zf, nullptr);
	unzClose(zipfile);
}
