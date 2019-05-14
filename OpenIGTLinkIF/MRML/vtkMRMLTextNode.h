/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLTextNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/

#ifndef __vtkMRMLTextNode_h
#define __vtkMRMLTextNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// MRML includes
#include <vtkMRMLStorableNode.h>

// VTK includes
#include <vtkStdString.h>

// STD includes
#include <sstream>

class  VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLTextNode : public vtkMRMLStorableNode
{
public:
  enum
  {
    ENCODING_US_ASCII = 3,
    ENCODING_ISO_8859_1 = 4,
    ENCODING_LATIN1 = ENCODING_ISO_8859_1 // alias
                      // see other codes at http://www.iana.org/assignments/character-sets/character-sets.xhtml
  };

  static vtkMRMLTextNode* New();
  vtkTypeMacro(vtkMRMLTextNode, vtkMRMLStorableNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual vtkMRMLNode* CreateNodeInstance() override;

  ///
  /// Set node attributes
  virtual void ReadXMLAttributes(const char** atts) override;

  ///
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) override;

  ///
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode* node) override;

  ///
  /// Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() override { return "Text"; };

  ///
  /// Set text encoding
  virtual void SetText(const char* text);
  vtkGetStringMacro(Text);

  ///
  /// Set encoding of the text
  /// For character encoding, please refer IANA Character Sets
  /// (http://www.iana.org/assignments/character-sets/character-sets.xhtml)
  /// Default is US-ASCII (ANSI-X3.4-1968; MIBenum = 3).
  vtkSetMacro(Encoding, int);
  vtkGetMacro(Encoding, int);

  /// Force the use of a storage node, regardless of text length.
  /// By default, a storage node will only be used for nodes that have been read from file (drag and drop),
  /// or for nodes that have text longer than 250 characters.
  /// This option should be also be enabled for nodes with highly structured text (such as XML) that would
  /// not be good to have in the MRML.
  vtkSetMacro(ForceStorageNode, bool);
  vtkGetMacro(ForceStorageNode, bool);

  /// Create a storage node for this node type.
  /// If it returns nullptr then it means the node can be stored
  /// in the scene (in XML), without using a storage node.
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode() override;

  /// Determines the most appropriate storage node class for the
  /// provided file name and node content.
  virtual std::string GetDefaultStorageNodeClassName(const char* filename=nullptr) override;

  enum
  {
    TextModifiedEvent = 30001,
  };

protected:
  vtkMRMLTextNode();
  ~vtkMRMLTextNode();
  vtkMRMLTextNode(const vtkMRMLTextNode&);
  void operator=(const vtkMRMLTextNode&);

  char* Text;
  int Encoding;
  bool ForceStorageNode;
};

#endif
