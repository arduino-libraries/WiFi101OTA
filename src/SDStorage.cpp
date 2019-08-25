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

#include "SDStorage.h"

#ifdef HAS_SD

#define UPDATE_FILE "UPDATE.BIN"

static inline void reboot() {
#ifdef __AVR__

#else
  NVIC_SystemReset();
#endif
}

int SDStorageClass::open(int length)
{
  (void)length;
  _file = SD.open(UPDATE_FILE, FILE_WRITE);

  if (!_file) {
    return 0;
  }

  return 1;
}

size_t SDStorageClass::write(uint8_t b)
{
  return _file.write(b);
}

void SDStorageClass::close()
{
  _file.close();
}

void SDStorageClass::clear()
{
  SD.remove(UPDATE_FILE);
}

void SDStorageClass::apply()
{
  // just reset, SDU copies the data to flash
  reboot();
}

SDStorageClass SDStorage;

#endif