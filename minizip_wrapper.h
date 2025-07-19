#pragma once

#include <QtCore/qpromise.h>
#include "fileinfo.h"

void findTextInZip(QPromise<FileInfo> &promise,
				   const QString &zipPath,
				   const std::string &search_str);
