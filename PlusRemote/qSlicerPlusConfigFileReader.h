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
  and was supported through CANARIE's Research Software Program, and Cancer
  Care Ontario.

==============================================================================*/

#ifndef __qSlicerPlusConfigFileReader_h
#define __qSlicerPlusConfigFileReader_h

// SlicerQt includes
#include "qSlicerFileReader.h"
class qSlicerPlusConfigFileReaderPrivate;

//-----------------------------------------------------------------------------
class qSlicerPlusConfigFileReader
  : public qSlicerFileReader
{
  Q_OBJECT
public:
  typedef qSlicerFileReader Superclass;
  qSlicerPlusConfigFileReader(QObject* parent = 0);
  virtual ~qSlicerPlusConfigFileReader();

  virtual qSlicerIOOptions* options() const override;
  virtual QString description() const override;
  virtual IOFileType fileType() const override;
  virtual QStringList extensions() const override;
  virtual bool load(const IOProperties& properties) override;

protected:
  QScopedPointer< qSlicerPlusConfigFileReaderPrivate > d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlusConfigFileReader);
  Q_DISABLE_COPY(qSlicerPlusConfigFileReader);
};

#endif
