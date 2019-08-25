/*
  Copyright (c) 2018 Arduino LLC.  All right reserved.

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

#include "NINAStorageRaw.h"

#ifdef HAS_NINA

#define UPDATE_FILE "/fs/UPDATE.BIN"

static inline void reboot() {
#ifdef __AVR__
  /* Write boot request */
  USERROW.USERROW31 = 0xEB;
  _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
  while(NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm);

  _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
#else
  NVIC_SystemReset();
#endif
}

int NINAStorageRawClass::open(int contentLength)
{
  if (WiFiStorage.exists(UPDATE_FILE)) {
    WiFiStorage.remove(UPDATE_FILE);
  }

  _file = new WiFiStorageFile(UPDATE_FILE);

  return 1;
}

static int buffer_index = 0;
static uint8_t buffer[2048];

size_t NINAStorageRawClass::write(uint8_t b)
{
  // buffer bytes in 2Kbytes chunks, otherwise this thing is slow as hell
  if (buffer_index < sizeof(buffer)) {
    buffer[buffer_index] = b;
    buffer_index++;
  }
  if (buffer_index >= sizeof(buffer)) {
    _file->write(buffer, sizeof(buffer));
    buffer_index = 0;
  }
  return 1;
}

void NINAStorageRawClass::close()
{
  if (buffer_index > 0) {
    _file->write(buffer, buffer_index);
  }
  _file->close();
}

void NINAStorageRawClass::clear()
{
  WiFiStorage.remove(UPDATE_FILE);
}

void NINAStorageRawClass::apply()
{
  reboot();
}

void NINAStorageRawClass::download(String url)
{
  WiFiStorage.download(url, "UPDATE.BIN");
}

long NINAStorageRawClass::maxSize()
{
#ifdef __AVR__
  return (0xFFFF - 0x3FFF - 0x100);
#else
  return ((256 * 1024) - 0x2000);
#endif
}

NINAStorageRawClass NINAStorageRaw;

#endif