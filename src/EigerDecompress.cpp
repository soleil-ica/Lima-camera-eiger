//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2015
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
#include "lz4.h"

#include "EigerDecompress.h"
#include "EigerStream.h"

#include "processlib/LinkTask.h"
#include "processlib/ProcessExceptions.h"

using namespace lima;
using namespace lima::Eiger;

class _DecompressTask : public LinkTask
{
  DEB_CLASS_NAMESPC(DebModCamera,"_DecompressTask","Eiger");
public:
  _DecompressTask(Stream& stream) : m_stream(stream) {}
  virtual Data process(Data&);

private:
  Stream& m_stream;
};

void _expend(void *src,Data& dst)
{
  int nbItems = dst.size() / dst.depth();
  unsigned short* src_data = (unsigned short*)src;
  unsigned int* dst_data = (unsigned int*)dst.data();
  while(nbItems)
    {
      *dst_data = unsigned(*src_data);
      ++dst_data,++src_data,--nbItems;
    }
  dst.type = Data::UINT32;
}

Data _DecompressTask::process(Data& src)
{
  void *msg_data;
  size_t msg_size;
  int depth;
  if(!m_stream.get_msg(src.data(),msg_data,msg_size,depth))
    throw ProcessException("_DecompressTask: can't find compressed message");
  void* dst;
  int size;
  if(src.depth() == 4 && depth == 2)
    {
    if(posix_memalign(&dst,16,src.size() / 2))
	throw ProcessException("Can't allocate temporary memory");
    size = src.size() / 2;
    }
  else
    dst = src.data(),size = src.size();

  int return_code = LZ4_decompress_fast((const char*)msg_data,(char*)dst,size);
  if(return_code < 0)
    {
      if(src.depth() == 4 && depth == 2) free(dst);

      char ErrorBuff[1024];
      snprintf(ErrorBuff,sizeof(ErrorBuff),
	       "_DecompressTask: decompression failed, (error code: %d) (data size %d)",
	       return_code,src.size());
      throw ProcessException(ErrorBuff);
    }
  if(src.depth() == 4 && depth == 2)
    {
      _expend(dst,src);
      free(dst);
    }
  return src;
}

Decompress::Decompress(Stream& stream) :
  m_decompress_task(new _DecompressTask(stream))
{
}

Decompress::~Decompress()
{
  m_decompress_task->unref();
}

LinkTask* Decompress::getReconstructionTask()
{
  return m_decompress_task;
}

void Decompress::setActive(bool active)
{
  reconstructionChange(active ? m_decompress_task : NULL);
}
