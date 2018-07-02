/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Kyle Sunderland, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#include "qMRMLPlusServerLauncherRemoteWidgetPlugin.h"
#include "qMRMLPlusServerLauncherRemoteWidget.h"

//------------------------------------------------------------------------------
qMRMLPlusServerLauncherRemoteWidgetPlugin::qMRMLPlusServerLauncherRemoteWidgetPlugin(QObject *_parent)
  : QObject(_parent)
{
}

//------------------------------------------------------------------------------
QWidget *qMRMLPlusServerLauncherRemoteWidgetPlugin::createWidget(QWidget *_parent)
{
  qMRMLPlusServerLauncherRemoteWidget* _widget
    = new qMRMLPlusServerLauncherRemoteWidget(_parent);
  return _widget;
}

//------------------------------------------------------------------------------
QString qMRMLPlusServerLauncherRemoteWidgetPlugin::domXml() const
{
  return "<widget class=\"qMRMLPlusServerLauncherRemoteWidget\" \
          name=\"PlusServerLauncherRemoteWidget\">\n"
          "</widget>\n";
}

//------------------------------------------------------------------------------
QString qMRMLPlusServerLauncherRemoteWidgetPlugin::includeFile() const
{
  return "qMRMLPlusServerLauncherRemoteWidget.h";
}

//------------------------------------------------------------------------------
bool qMRMLPlusServerLauncherRemoteWidgetPlugin::isContainer() const
{
  return false;
}

//------------------------------------------------------------------------------
QString qMRMLPlusServerLauncherRemoteWidgetPlugin::name() const
{
  return "qMRMLPlusServerLauncherRemoteWidget";
}
