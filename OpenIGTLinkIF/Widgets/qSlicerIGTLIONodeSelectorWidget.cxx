
// Qt includes
#include <QButtonGroup>

// OpenIGTLinkIF GUI includes
#include "qSlicerIGTLIONodeSelectorWidget.h"
#include "ui_qSlicerIGTLIONodeSelectorWidget.h"

#include "qMRMLIGTLIOTreeView.h"

// OpenIGTLinkIF MRML includes
#include "vtkMRMLIGTLConnectorNode.h"

//------------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_OpenIGTLinkIF
class qSlicerIGTLIONodeSelectorWidgetPrivate : public Ui_qSlicerIGTLIONodeSelectorWidget
{
  Q_DECLARE_PUBLIC(qSlicerIGTLIONodeSelectorWidget);
protected:
  qSlicerIGTLIONodeSelectorWidget* const q_ptr;
public:
  qSlicerIGTLIONodeSelectorWidgetPrivate(qSlicerIGTLIONodeSelectorWidget& object);
  void init();

  vtkMRMLIGTLConnectorNode * ConnectorNode;
  int Direction;
  vtkMRMLNode * DataNode;
};

//------------------------------------------------------------------------------
qSlicerIGTLIONodeSelectorWidgetPrivate::qSlicerIGTLIONodeSelectorWidgetPrivate(qSlicerIGTLIONodeSelectorWidget& object)
  : q_ptr(&object)
{
  //this->CurrentNode = NULL;
  this->ConnectorNode = 0;
  this->Direction = qSlicerIGTLIONodeSelectorWidget::UNDEFINED;
}

//------------------------------------------------------------------------------
void qSlicerIGTLIONodeSelectorWidgetPrivate::init()
{
  Q_Q(qSlicerIGTLIONodeSelectorWidget);
  this->setupUi(q);

  QObject::connect(this->AddNodeButton, SIGNAL(clicked()),
                   q, SLOT(onAddNodeButtonClicked()));
  QObject::connect(this->RemoveNodeButton, SIGNAL(clicked()),
                   q, SLOT(onRemoveNodeButtonClicked()));
  QObject::connect(this->SendButton, SIGNAL(clicked()),
                   q, SLOT(onSendButtonClicked()));

}

//------------------------------------------------------------------------------
qSlicerIGTLIONodeSelectorWidget::qSlicerIGTLIONodeSelectorWidget(QWidget *_parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerIGTLIONodeSelectorWidgetPrivate(*this))
{
  Q_D(qSlicerIGTLIONodeSelectorWidget);
  d->init();
}

//------------------------------------------------------------------------------
qSlicerIGTLIONodeSelectorWidget::~qSlicerIGTLIONodeSelectorWidget()
{
}

//------------------------------------------------------------------------------
void qSlicerIGTLIONodeSelectorWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerIGTLIONodeSelectorWidget);
  if (d->NodeSelector)
    {
    d->NodeSelector->setMRMLScene(scene);
    }
}


//------------------------------------------------------------------------------

void qSlicerIGTLIONodeSelectorWidget::updateEnabledStatus(int type, vtkMRMLIGTLConnectorNode* cnode, int dir, vtkMRMLNode* dnode)
{
  Q_D(qSlicerIGTLIONodeSelectorWidget);

  if (type == qMRMLIGTLIOTreeView::TYPE_ROOT ||
      type == qMRMLIGTLIOTreeView::TYPE_UNKNOWN ||
      type == qMRMLIGTLIOTreeView::TYPE_CONNECTOR)
    {
    d->AddNodeButton->setEnabled(false);
    d->RemoveNodeButton->setEnabled(false);
    d->NodeSelector->setEnabled(false);
    d->SendButton->setEnabled(false);
    }
  else if (type == qMRMLIGTLIOTreeView::TYPE_STREAM)
    {
    d->AddNodeButton->setEnabled(true);
    d->RemoveNodeButton->setEnabled(false);
    d->NodeSelector->setEnabled(true);
    d->SendButton->setEnabled(false);
    }
  else
    {
    d->AddNodeButton->setEnabled(true);
    d->RemoveNodeButton->setEnabled(true);
    d->NodeSelector->setEnabled(true);
    if (dir == 2) // outgoing
      {
      d->SendButton->setEnabled(true);
      }
    else
      {
      d->SendButton->setEnabled(false);
      }
    }

  d->ConnectorNode = cnode;
  d->Direction = dir;
  d->DataNode = dnode;
  
}


//------------------------------------------------------------------------------
void qSlicerIGTLIONodeSelectorWidget::onAddNodeButtonClicked()
{
  Q_D(qSlicerIGTLIONodeSelectorWidget);

  vtkMRMLNode* node = d->NodeSelector->currentNode();
  if (node == 0)
    {
    return;
    }

  if (d->ConnectorNode)
    {
    if (d->Direction == 2)
      {
      d->ConnectorNode->RegisterOutgoingMRMLNode(node);
      }
    }
  
  //emit addNode(node);
}


//------------------------------------------------------------------------------
void qSlicerIGTLIONodeSelectorWidget::onRemoveNodeButtonClicked()
{
  Q_D(qSlicerIGTLIONodeSelectorWidget);

  if (d->ConnectorNode && d->DataNode)
    {
    if (d->Direction == 1)
      {
      d->ConnectorNode->UnregisterIncomingMRMLNode(d->DataNode);
      }
    else if (d->Direction == 2)
      {
      d->ConnectorNode->UnregisterOutgoingMRMLNode(d->DataNode);
      }
    }

}


//------------------------------------------------------------------------------
void qSlicerIGTLIONodeSelectorWidget::onSendButtonClicked()
{
  Q_D(qSlicerIGTLIONodeSelectorWidget);

  if (d->ConnectorNode && d->DataNode)
    {
    if (d->Direction == 2)
      {
      d->ConnectorNode->PushNode(d->DataNode);
      }
    }

}


