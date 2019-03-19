/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QMap>
#include <QMimeData>
#include <QSharedPointer>
#include <QStack>
#include <QStringList>
#include <QVector>

// OpenIGTLinkIF GUI includes
#include "qMRMLSceneIGTLConnectorModel.h"

// qMRMLWidgets includes
#include <qMRMLSceneDisplayableModel.h>

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLDisplayableNode.h>

// VTK includes
#include <vtkVariantArray.h>

//------------------------------------------------------------------------------
qMRMLSceneIGTLConnectorModel::qMRMLSceneIGTLConnectorModel(QObject* vparent)
  : qMRMLSceneDisplayableModel(vparent)
{
  this->setIDColumn(-1);
  this->setHorizontalHeaderLabels(
    QStringList() << "Name" << "Type" << "Status" << "Hostname" << "Port");
}

//------------------------------------------------------------------------------
qMRMLSceneIGTLConnectorModel::~qMRMLSceneIGTLConnectorModel()
{
}

//------------------------------------------------------------------------------
QStandardItem* qMRMLSceneIGTLConnectorModel::insertNode(vtkMRMLNode* node, QStandardItem* parent, int row)
{
  QStandardItem* insertedItem = this->Superclass::insertNode(node, parent, row);
  if (this->listenNodeModifiedEvent() &&
      node->IsA("vtkMRMLIGTLConnectorNode"))
  {
    qvtkConnect(node, vtkMRMLIGTLConnectorNode::ConnectedEvent,
                this, SLOT(onMRMLNodeModified(vtkObject*)));
    qvtkConnect(node, vtkMRMLIGTLConnectorNode::DisconnectedEvent,
                this, SLOT(onMRMLNodeModified(vtkObject*)));
    qvtkConnect(node, vtkMRMLIGTLConnectorNode::ActivatedEvent,
                this, SLOT(onMRMLNodeModified(vtkObject*)));
    qvtkConnect(node, vtkMRMLIGTLConnectorNode::DeactivatedEvent,
                this, SLOT(onMRMLNodeModified(vtkObject*)));
  }
  return insertedItem;
}

//------------------------------------------------------------------------------
void qMRMLSceneIGTLConnectorModel::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  //int oldChecked = node->GetSelected();
  vtkMRMLIGTLConnectorNode* cnode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (!cnode)
  {
    return;
  }
  this->qMRMLSceneDisplayableModel::updateNodeFromItemData(node, item);

  switch (item->column())
  {
    case qMRMLSceneIGTLConnectorModel::NameColumn:
    {
      cnode->SetName(item->text().toLatin1());
      break;
    }
    case qMRMLSceneIGTLConnectorModel::TypeColumn:
    {
    }
    case qMRMLSceneIGTLConnectorModel::StatusColumn:
    {
    }
    case qMRMLSceneIGTLConnectorModel::HostnameColumn:
    {
    }
    case qMRMLSceneIGTLConnectorModel::PortColumn:
    {
    }
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void qMRMLSceneIGTLConnectorModel::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  vtkMRMLIGTLConnectorNode* cnode = vtkMRMLIGTLConnectorNode::SafeDownCast(node);
  if (!cnode)
  {
    return;
  }
  switch (column)
  {
    case qMRMLSceneIGTLConnectorModel::NameColumn:
    {
      item->setText(cnode->GetName());
      break;
    }
    case qMRMLSceneIGTLConnectorModel::TypeColumn:
    {
      switch (cnode->GetType())
      {
        case vtkMRMLIGTLConnectorNode::TypeClient:
          item->setText(tr("C"));
          break;
        case vtkMRMLIGTLConnectorNode::TypeServer:
          item->setText(tr("S"));
          break;
        default:
          item->setText(tr("?"));
      }
      break;
    }
    case qMRMLSceneIGTLConnectorModel::StatusColumn:
    {
      switch (cnode->GetState())
      {
        case vtkMRMLIGTLConnectorNode::StateOff:
          item->setText(tr("OFF"));
          break;
        case vtkMRMLIGTLConnectorNode::StateWaitConnection:
          item->setText(tr("WAIT"));
          break;
        case vtkMRMLIGTLConnectorNode::StateConnected:
          item->setText(tr("ON"));
          break;
        default:
          item->setText("");
      }
      break;
    }
    case qMRMLSceneIGTLConnectorModel::HostnameColumn:
    {
      if (cnode->GetType() == vtkMRMLIGTLConnectorNode::TypeClient)
      {
        item->setText(QString(cnode->GetServerHostname()));
      }
      else
      {
        item->setText(QString("--"));
      }
      break;
    }
    case qMRMLSceneIGTLConnectorModel::PortColumn:
    {
      item->setText(QString("%1").arg(cnode->GetServerPort()));
      break;
    }
    default:
      break;
  }
}

//------------------------------------------------------------------------------
QFlags<Qt::ItemFlag> qMRMLSceneIGTLConnectorModel::nodeFlags(vtkMRMLNode* node, int column)const
{
  QFlags<Qt::ItemFlag> flags = this->qMRMLSceneDisplayableModel::nodeFlags(node, column);
  // remove the ItemIsEditable flag from any possible item (typically at column 0)
  flags = flags & ~Qt::ItemIsEditable;
  // and set it to the right column
  switch (column)
  {
    //case qMRMLSceneIGTLConnectorModel::TextColumn:
    //  flags = flags | Qt::ItemIsEditable;
    //  break;
    default:
      break;
  }
  return flags;
}

//------------------------------------------------------------------------------
vtkMRMLNode* qMRMLSceneIGTLConnectorModel::parentNode(vtkMRMLNode* node)const
{
  if (node == NULL)
  {
    return 0;
  }

  // MRML Displayable nodes (inherits from transformable)
  vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(node);
  vtkMRMLDisplayableHierarchyNode* displayableHierarchyNode = NULL;
  if (displayableNode &&
      displayableNode->GetScene() &&
      displayableNode->GetID())
  {
    // get the displayable hierarchy node associated with this displayable node
    displayableHierarchyNode = vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(displayableNode->GetScene(), displayableNode->GetID());

    if (displayableHierarchyNode)
    {
      if (displayableHierarchyNode->GetHideFromEditors())
      {
        // this is a hidden hierarchy node, so we do not want to display it
        // instead, we will return the parent of the hidden hierarchy node
        // to be used as the parent for the displayableNode
        return displayableHierarchyNode->GetParentNode();
      }
      return displayableHierarchyNode;
    }
  }
  if (displayableHierarchyNode == NULL)
  {
    // the passed in node might have been a hierarchy node instead, try to
    // cast it
    displayableHierarchyNode = vtkMRMLDisplayableHierarchyNode::SafeDownCast(node);
  }
  if (displayableHierarchyNode)
  {
    // return it's parent
    return displayableHierarchyNode->GetParentNode();
  }
  return 0;
}


//-----------------------------------------------------------------------------
/// Set and observe the logic
//-----------------------------------------------------------------------------
void qMRMLSceneIGTLConnectorModel::setLogic(vtkSlicerOpenIGTLinkIFLogic* logic)
{
  if (!logic)
  {
    return;
  }
  this->Logic = logic;
}
