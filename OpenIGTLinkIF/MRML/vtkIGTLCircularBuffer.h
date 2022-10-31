/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLCircularBuffer.h $
  Date:      $Date: 2009-08-12 21:30:38 -0400 (Wed, 12 Aug 2009) $
  Version:   $Revision: 10236 $

==========================================================================*/

#ifndef __vtkIGTLCircularBuffer_h
#define __vtkIGTLCircularBuffer_h

// VTK includes
#include <vtkObject.h>

// OpenIGTLink includes
#include <igtlMessageBase.h>

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// STD includes
#include <string>
#include <mutex>

#define IGTLCB_CIRC_BUFFER_SIZE    3

class vtkIGTLCircularBuffer : public vtkObject
{
public:

  static vtkIGTLCircularBuffer* New();
  vtkTypeMacro(vtkIGTLCircularBuffer, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  int GetNumberOfBuffer() { return IGTLCB_CIRC_BUFFER_SIZE; }

  int            StartPush();
  void           EndPush();
  igtl::MessageBase::Pointer GetPushBuffer();

  int            StartPull();
  void           EndPull();
  igtl::MessageBase::Pointer GetPullBuffer();

  int            IsUpdated() { return this->UpdateFlag; };

protected:
  vtkIGTLCircularBuffer();
  virtual ~vtkIGTLCircularBuffer();

protected:
  std::mutex         Mutex;
  int                Last;        // updated by connector thread
  int                InPush;      // updated by connector thread
  int                InUse;       // updated by main thread

  int                UpdateFlag;  // non-zero if updated since StartPull() has called

  std::string        DeviceType[IGTLCB_CIRC_BUFFER_SIZE];
  long long          Size[IGTLCB_CIRC_BUFFER_SIZE];
  unsigned char*     Data[IGTLCB_CIRC_BUFFER_SIZE];

  igtl::MessageBase::Pointer Messages[IGTLCB_CIRC_BUFFER_SIZE];

};

#endif //__vtkIGTLCircularBuffer_h
