/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __vtkMRMLImageMetaElement_h
#define __vtkMRMLImageMetaElement_h

// OpenIGTLinkIF MRML includes
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLImageMetaElement
{
public:
    std::string   Name;        /* name / description (< 64 bytes)*/
    std::string   DeviceName;  /* device name to query the IMAGE and COLORT */
    std::string   Modality;    /* modality name (< 32 bytes) */
    std::string   PatientName; /* patient name (< 64 bytes) */
    std::string   PatientID;   /* patient ID (MRN etc.) (< 64 bytes) */
    double        TimeStamp;   /* scan time */
    int           Size[3];     /* entire image volume size */
    unsigned char ScalarType;  /* scalar type. see scalar_type in IMAGE message */
};

#endif
