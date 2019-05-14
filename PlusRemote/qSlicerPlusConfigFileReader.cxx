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
  and was supported through CANARIE's Research Software Program, and Cancer
  Care Ontario.

==============================================================================*/

// Qt includes
#include <QDebug>

// OpenIGTLinkIF includes
#include "vtkMRMLTextNode.h"
#include "vtkMRMLTextStorageNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// PlusRemote includes
#include "qSlicerPlusConfigFileReader.h"
#include "qSlicerPlusConfigFileIOOptionsWidget.h"

// PlusRemote MRML includes
#include "vtkMRMLPlusServerNode.h"
#include "vtkMRMLPlusServerLauncherNode.h"

// VTK includes
#include <vtkSmartPointer.h>
#include <vtksys/SystemTools.hxx>

// std includes
#include <cctype>
#include <iostream>
#include <string>

//-----------------------------------------------------------------------------
class qSlicerPlusConfigFileReaderPrivate
{
};

//-----------------------------------------------------------------------------
qSlicerPlusConfigFileReader::qSlicerPlusConfigFileReader(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerPlusConfigFileReaderPrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerPlusConfigFileReader::~qSlicerPlusConfigFileReader()
{
}


//-----------------------------------------------------------------------------
qSlicerIOOptions* qSlicerPlusConfigFileReader::options()const
{
  qSlicerIOOptionsWidget* options = new qSlicerPlusConfigFileIOOptionsWidget;
  options->setMRMLScene(this->mrmlScene());
  return options;
}

//-----------------------------------------------------------------------------
QString qSlicerPlusConfigFileReader::description() const
{
  return "Plus config file";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerPlusConfigFileReader::fileType() const
{
  return "Plus config file";
}

//-----------------------------------------------------------------------------
QStringList qSlicerPlusConfigFileReader::extensions() const
{
  QStringList supportedExtensions = QStringList();
  supportedExtensions << "Plus config file (*.plus.xml)";
  return supportedExtensions;
}

//-----------------------------------------------------------------------------
bool qSlicerPlusConfigFileReader::load(const IOProperties& properties)
{
  Q_D(qSlicerPlusConfigFileReader);
  if (!properties.contains("fileName"))
  {
    qCritical() << Q_FUNC_INFO << " did not receive fileName property";
    return false;
  }
  std::string fileName = properties["fileName"].toString().toStdString();
  bool autoConnect = properties["autoConnect"].toBool();
  std::string launcherNodeID = properties["launcherNodeID"].toString().toStdString();

  std::string textNodeName = vtksys::SystemTools::GetFilenameWithoutExtension(fileName);
  textNodeName = this->mrmlScene()->GetUniqueNameByString(textNodeName.c_str());
  vtkSmartPointer<vtkMRMLTextNode> textNode = vtkMRMLTextNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLTextNode", textNodeName));
  if (!textNode)
  {
    return false;
  }
  textNodeName = textNode->GetName();
  textNode->SetAttribute(vtkMRMLPlusServerNode::SERVER_ID_ATTRIBUTE_NAME.c_str(), textNodeName.c_str());

  vtkSmartPointer<vtkMRMLTextStorageNode> storageNode = vtkMRMLTextStorageNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLTextStorageNode"));
  if (!storageNode)
  {
    this->mrmlScene()->RemoveNode(textNode);
    return false;
  }

  textNode->SetAndObserveStorageNodeID(storageNode->GetID());
  storageNode->SetFileName(fileName.c_str());


  bool success = (storageNode->ReadData(textNode) != -1);

  vtkSmartPointer<vtkMRMLPlusServerNode> serverNode = nullptr;
  if (success)
  {
    serverNode = vtkMRMLPlusServerNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLPlusServerNode", textNodeName+"_Server"));
    if (serverNode)
    {
      serverNode->SetAndObserveConfigNode(textNode);
    }
    else
    {
      qCritical() << Q_FUNC_INFO << "could not create server node";
      success = false;
    }
  }

  vtkSmartPointer<vtkMRMLPlusServerLauncherNode> launcherNode;
  if (success)
  {
    if (launcherNodeID.empty())
    {
      launcherNode = vtkMRMLPlusServerLauncherNode::SafeDownCast(this->mrmlScene()->AddNewNodeByClass("vtkMRMLPlusServerLauncherNode"));
    }
    else
    {
      launcherNode = vtkMRMLPlusServerLauncherNode::SafeDownCast(this->mrmlScene()->GetNodeByID(launcherNodeID));
    }

    if (launcherNode)
    {
      launcherNode->AddAndObserveServerNode(serverNode);
    }
    else
    {
      if (launcherNodeID.empty())
      {
        qCritical() << Q_FUNC_INFO << "could not create launcher node";
      }
      else
      {
        qCritical() << Q_FUNC_INFO << "could not find launcher node with ID: " << launcherNodeID.c_str();
      }
      success = false;
    }
  }

  if (!success)
  {
    this->mrmlScene()->RemoveNode(textNode);
    this->mrmlScene()->RemoveNode(storageNode);
    this->mrmlScene()->RemoveNode(launcherNode);
    this->mrmlScene()->RemoveNode(serverNode);
    return false;
  }

  if (autoConnect)
  {
    serverNode->StartServer();
  }
  return true;
}
