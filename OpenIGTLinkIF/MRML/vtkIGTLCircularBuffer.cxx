/*==========================================================================

  Portions (c) Copyright 2008-2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $HeadURL: http://svn.slicer.org/Slicer3/trunk/Modules/OpenIGTLinkIF/vtkIGTLCircularBuffer.cxx $
  Date:      $Date: 2009-08-12 21:30:38 -0400 (Wed, 12 Aug 2009) $
  Version:   $Revision: 10236 $

==========================================================================*/

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkIGTLCircularBuffer.h>

// VTKSYS includes
#include <vtksys/SystemTools.hxx>

// OpenIGTLink includes
#include <igtlMessageBase.h>

// STD includes
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLCircularBuffer);

//---------------------------------------------------------------------------
vtkIGTLCircularBuffer::vtkIGTLCircularBuffer()
{
  this->Mutex.lock();
  // Allocate Circular buffer for the new device
  this->InUse = -1;
  this->Last  = -1;
  for (int i = 0; i < IGTLCB_CIRC_BUFFER_SIZE; i ++)
  {
    this->DeviceType[i] = "";
    this->Size[i]       = 0;
    this->Data[i]       = NULL;
    this->Messages[i] = igtl::MessageBase::New();
    this->Messages[i]->InitPack();
  }

  this->UpdateFlag = 0;
  this->Mutex.unlock();
}


//---------------------------------------------------------------------------
vtkIGTLCircularBuffer::~vtkIGTLCircularBuffer()
{
  this->Mutex.lock();
  this->InUse = -1;
  this->Last  = -1;
  this->Mutex.unlock();

  for (int i = 0; i < IGTLCB_CIRC_BUFFER_SIZE; i ++)
  {
    if (this->Data[i] != NULL)
    {
      delete this->Data[i];
    }
  }
}


//---------------------------------------------------------------------------
void vtkIGTLCircularBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}


//---------------------------------------------------------------------------
// Functions to push data into the circular buffer (for receiving thread)
//
//   StartPush() :     Prepare to push data
//   GetPushBuffer():  Get MessageBase buffer from the circular buffer
//   EndPush() :       Finish pushing data. The data becomes ready to
//                     be read by monitor thread.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int vtkIGTLCircularBuffer::StartPush()
{
  this->Mutex.lock();
  this->InPush = (this->Last + 1) % IGTLCB_CIRC_BUFFER_SIZE;
  if (this->InPush == this->InUse)
  {
    this->InPush = (this->Last + 1) % IGTLCB_CIRC_BUFFER_SIZE;
  }
  this->Mutex.unlock();

  return this->InPush;
}

//---------------------------------------------------------------------------
igtl::MessageBase::Pointer vtkIGTLCircularBuffer::GetPushBuffer()
{
  return this->Messages[this->InPush];
}

//---------------------------------------------------------------------------
void vtkIGTLCircularBuffer::EndPush()
{
  this->Mutex.lock();
  this->Last = this->InPush;
  this->UpdateFlag = 1;
  this->Mutex.unlock();
}


//---------------------------------------------------------------------------
// Functions to pull data into the circular buffer (for monitor thread)
//
//   StartPull() :     Prepare to pull data
//   GetPullBuffer():  Get MessageBase buffer from the circular buffer
//   EndPull() :       Finish pulling data
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
int vtkIGTLCircularBuffer::StartPull()
{
  this->Mutex.lock();
  this->InUse = this->Last;
  this->UpdateFlag = 0;
  this->Mutex.unlock();
  return this->Last;   // return -1 if it is not available
}


//---------------------------------------------------------------------------
igtl::MessageBase::Pointer vtkIGTLCircularBuffer::GetPullBuffer()
{
  return this->Messages[this->InUse];
}


//---------------------------------------------------------------------------
void vtkIGTLCircularBuffer::EndPull()
{
  this->Mutex.lock();
  this->InUse = -1;
  this->Mutex.unlock();
}
