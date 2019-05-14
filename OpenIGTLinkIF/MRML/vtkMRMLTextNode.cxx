/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLTextNode.cxx,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/

// SlicerOpenIGTLink MRML includes
#include "vtkMRMLTextNode.h"
#include "vtkMRMLTextStorageNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include "vtkObjectFactory.h"
#include "vtkXMLUtilities.h"

const int LENGTH_BEFORE_STORAGE_NODE = 256;

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTextNode);

//-----------------------------------------------------------------------------
vtkMRMLTextNode::vtkMRMLTextNode()
  : Text(NULL)
  , Encoding(ENCODING_US_ASCII)
  , ForceStorageNode(false)
{
}

//-----------------------------------------------------------------------------
vtkMRMLTextNode::~vtkMRMLTextNode()
{
  this->SetText(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLTextNode::SetText(const char* text)
{
  vtkDebugMacro( << this->GetClassName() << " (" << this << "): setting Text to " << (text ? text : "(null)"));

  if (this->Text == nullptr && text == nullptr)
  {
    return;
  }
  if (this->Text && text && (!strcmp(this->Text, text)))
  {
    return;
  }

  delete[] this->Text;
  if (text)
  {
    size_t n = strlen(text) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (text);
    this->Text = cp1;
    do
    {
      *cp1++ = *cp2++;
    }
    while (--n);
  }
  else
  {
    this->Text = nullptr;
  }

  this->InvokeCustomModifiedEvent(vtkMRMLTextNode::TextModifiedEvent);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLTextNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStringMacro(text, Text);
  vtkMRMLReadXMLIntMacro(encoding, Encoding);
  vtkMRMLReadXMLEndMacro();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTextNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkMRMLWriteXMLBeginMacro(of);
  if (!this->GetStorageNode())
  {
    vtkMRMLWriteXMLStringMacro(text, Text);
  }
  vtkMRMLWriteXMLIntMacro(encoding, Encoding);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLTextNode::Copy(vtkMRMLNode* anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  vtkMRMLCopyBeginMacro(anode);
  vtkMRMLCopyStringMacro(Text);
  vtkMRMLCopyIntMacro(Encoding);
  vtkMRMLCopyEndMacro();
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTextNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  vtkMRMLPrintBeginMacro(os, indent);
  vtkMRMLPrintStringMacro(Text);
  vtkMRMLPrintIntMacro(Encoding);
  vtkMRMLPrintEndMacro();
}

//---------------------------------------------------------------------------
std::string vtkMRMLTextNode::GetDefaultStorageNodeClassName(const char* filename)
{
  if (!this->Text || !this->Scene)
  {
    return nullptr;
  }

  if (!this->ForceStorageNode)
  {
    int length = strlen(this->Text);
    if (length < LENGTH_BEFORE_STORAGE_NODE)
    {
      return "";
    }
  }
  return "vtkMRMLTextStorageNode";
}

//---------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLTextNode::CreateDefaultStorageNode()
{
  if (!this->Text || !this->Scene)
  {
    return nullptr;
  }

  if (!this->ForceStorageNode)
  {
    int length = strlen(this->Text);
    if (length < LENGTH_BEFORE_STORAGE_NODE)
    {
      return nullptr;
    }
  }
  return vtkMRMLTextStorageNode::SafeDownCast(
    this->Scene->CreateNodeByClass(this->GetDefaultStorageNodeClassName().c_str()));
}
