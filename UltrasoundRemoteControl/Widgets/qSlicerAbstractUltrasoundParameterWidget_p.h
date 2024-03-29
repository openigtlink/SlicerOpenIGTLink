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

#ifndef __qSlicerAbstractUltrasoundParameterWidget_p_h
#define __qSlicerAbstractUltrasoundParameterWidget_p_h

#include <QTimer>

//-----------------------------------------------------------------------------
class qSlicerAbstractUltrasoundParameterWidgetPrivate
{
public:
  Q_DECLARE_PUBLIC(qSlicerAbstractUltrasoundParameterWidget);

public:
  qSlicerAbstractUltrasoundParameterWidgetPrivate(qSlicerAbstractUltrasoundParameterWidget* q);
  virtual void init();

protected:
  qSlicerAbstractUltrasoundParameterWidget* const q_ptr;

public:
  vtkWeakPointer<vtkMRMLIGTLConnectorNode> ConnectorNode{ nullptr };

  std::string           ParameterName{ "" };
  std::string           DeviceID{ "" };
  bool                  PushOnConnect{ false };

  igtlioCommandPointer  CmdSetParameter{ nullptr };
  igtlioCommandPointer  CmdGetParameter{ nullptr };

  bool                  InteractionInProgress{ false };

  QTimer                PeriodicParameterTimer;
};

#endif
