#ifndef __vtkMRMLTextNode_h
#define __vtkMRMLTextNode_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkStdString.h>

// STD includes
#include <sstream>

class  VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLTextNode : public vtkMRMLNode
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
  vtkTypeMacro(vtkMRMLTextNode, vtkMRMLNode);
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
  virtual const char* GetNodeTagName() override {return "Text";};

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

  enum
  {
    TextModifiedEvent  = 30001,
  };

protected:
  vtkMRMLTextNode();
  ~vtkMRMLTextNode();
  vtkMRMLTextNode(const vtkMRMLTextNode&);
  void operator=(const vtkMRMLTextNode&);

  char* Text;
  int Encoding;
};

#endif
