#include <QtWidgets/QApplication>

// Avoid tensorflow compile error hell
// TODO define this as part of "Deep learning ... libs.targets" (or props)
#define COMPILER_MSVC
#define NOMINMAX
