#ifndef __tessinterfaces_global__
#define __tessinterfaces_global__

#include <QtCore/qglobal.h>
#include "TessInterfaces_Words.h"

#if defined(__clang__) && defined(QT_ANNOTATE_ACCESS_SPECIFIER)
// Shiboken 的 Clang 解析阶段不处理 Windows 导入标记，避免嵌套接口触发解析器异常。
# define TESSINTERFACES_EXPORT
#elif !defined(BUILD_STATIC)
# if defined(TESSINTERFACES_LIB)
#  define TESSINTERFACES_EXPORT Q_DECL_EXPORT
# else
#  define TESSINTERFACES_EXPORT Q_DECL_IMPORT
# endif
#else
# define TESSINTERFACES_EXPORT
#endif

#endif
