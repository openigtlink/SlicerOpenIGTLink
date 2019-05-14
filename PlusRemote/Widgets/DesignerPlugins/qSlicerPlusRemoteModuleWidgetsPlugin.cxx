#include "qSlicerPlusRemoteModuleWidgetsPlugin.h"
#include "qMRMLPlusServerLauncherTableViewPlugin.h"

#include <QtGlobal>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  #include <QtPlugin>
  Q_EXPORT_PLUGIN2(customwidgetplugin, qSlicerPlusRemoteModuleWidgetsPlugin);
  Q_EXPORT_PLUGIN2(customwidgetplugin, qMRMLPlusServerLauncherTableViewPlugin);
#endif
