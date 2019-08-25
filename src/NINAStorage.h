/*
  Copyright (c) 2017 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _NINA_STORAGE_H_INCLUDED
#define _NINA_STORAGE_H_INCLUDED

#ifdef __has_include
  #if __has_include(<WiFiStorage.h>)
    #include <WiFiStorage.h>
    #define HAS_NINA 1
  #endif
#else
    #include <WiFiStorage.h>
    #define HAS_NINA 1
#endif

#ifdef HAS_NINA

#include "OTAStorage.h"

class NINAStorageClass : public OTAStorage {
public:
  virtual int open(int length);
  virtual size_t write(uint8_t);
  virtual void close();
  virtual void clear();
  virtual void apply();
  virtual long maxSize();
  virtual void download(String url);
  virtual bool hasDownloadAPI() {
    return true;
  }

private:
  WiFiStorageFile* _file;
};

extern NINAStorageClass NINAStorage;

#endif
#endif
