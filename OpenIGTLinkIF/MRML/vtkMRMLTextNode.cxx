#include "vtkObjectFactory.h"
#include "vtkMRMLTextNode.h"
#include "vtkXMLUtilities.h"

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLTextNode);

//-----------------------------------------------------------------------------
vtkMRMLTextNode::vtkMRMLTextNode()
{
  this->Text = NULL;
  this->Encoding = ENCODING_US_ASCII;
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
  \
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

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
  {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "text"))
    {
      this->SetText(attValue);
    }
    else if (!strcmp(attName, "encoding"))
    {
      std::stringstream ss;
      ss << attValue;
      int encoding = 0;
      ss >> encoding;
      if (encoding >= 3)
      {
        this->SetEncoding(encoding);
      }
      else
      {
        vtkErrorMacro("Failed to set encoding. Invalid input value: " << attValue);
      }
    }
  }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTextNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  of << indent << " text=\"";
  if (this->GetText() != NULL)
  {
    // Write to XML, encoding special characters, such as " ' \ < > &
    vtkXMLUtilities::EncodeString(this->GetText(), VTK_ENCODING_NONE, of, VTK_ENCODING_NONE, 1 /* encode special characters */);
  }
  of << "\"";

  of << indent << " encoding=\"" << this->GetEncoding() << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLTextNode::Copy(vtkMRMLNode* anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  vtkMRMLTextNode* node = vtkMRMLTextNode::SafeDownCast(anode);

  this->SetText(node->GetText());
  this->SetEncoding(node->GetEncoding());

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLTextNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << "Text: " << ((this->GetText()) ? this->GetText() : "(none)") << "\n";
  os << "Encoding: " << this->GetEncoding() << "\n";
}

