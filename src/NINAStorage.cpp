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

#include "NINAStorage.h"

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

int NINAStorageClass::open(int contentLength)
{
  if (WiFiStorage.exists(UPDATE_FILE)) {
    WiFiStorage.remove(UPDATE_FILE);
  }

  WiFiStorageFile file = WiFiStorage.open(UPDATE_FILE);

  if (!file) {
    return 0;
  }

  _file = &file;

  return 1;
}

size_t NINAStorageClass::write(uint8_t b)
{
  int ret = _file->write(&b, 1);
  return ret;
}

void NINAStorageClass::close()
{
  _file->close();
}

void NINAStorageClass::clear()
{
  WiFiStorage.remove(UPDATE_FILE);
}

void NINAStorageClass::apply()
{
  WiFiDrv::applyOTA();
  reboot();
}

NINAStorageClass NINAStorage;
