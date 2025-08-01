/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "deskflow/IClipboard.h"

#include <vector>

//
// IClipboard
//

void IClipboard::unmarshall(IClipboard *clipboard, const std::string_view &data, Time time)
{
  assert(clipboard != nullptr);

  const char *index = data.data();

  if (clipboard->open(time)) {
    // clear existing data
    clipboard->empty();

    // read the number of formats
    const uint32_t numFormats = readUInt32(index);
    index += 4;

    // read each format
    for (uint32_t i = 0; i < numFormats; ++i) {
      // get the format id
      auto format = static_cast<IClipboard::Format>(readUInt32(index));
      index += 4;

      // get the size of the format data
      uint32_t size = readUInt32(index);
      index += 4;

      // save the data if it's a known format.  if either the client
      // or server supports more clipboard formats than the other
      // then one of them will get a format >= kNumFormats here.
      if (format < IClipboard::Format::TotalFormats) {
        clipboard->add(format, std::string(index, size));
      }
      index += size;
    }

    // done
    clipboard->close();
  }
}

std::string IClipboard::marshall(const IClipboard *clipboard)
{
  // return data format:
  // 4 bytes => number of formats included
  // 4 bytes => format enum
  // 4 bytes => clipboard data size n
  // n bytes => clipboard data
  // back to the second 4 bytes if there is another format

  assert(clipboard != nullptr);

  std::string data;
  static const auto totalClipboardFormats = static_cast<int>(Format::TotalFormats);
  std::vector<std::string> formatData;
  formatData.resize(totalClipboardFormats);
  // FIXME -- use current time
  if (clipboard->open(0)) {

    // compute size of marshalled data
    uint32_t size = 4;
    uint32_t numFormats = 0;
    for (uint32_t format = 0; format != totalClipboardFormats; ++format) {
      if (clipboard->has(static_cast<IClipboard::Format>(format))) {
        ++numFormats;
        formatData[format] = clipboard->get(static_cast<IClipboard::Format>(format));
        size += 4 + 4 + (uint32_t)formatData[format].size();
      }
    }

    // allocate space
    data.reserve(size);

    // marshall the data
    writeUInt32(&data, numFormats);
    for (uint32_t format = 0; format != totalClipboardFormats; ++format) {
      if (clipboard->has(static_cast<IClipboard::Format>(format))) {
        writeUInt32(&data, format);
        writeUInt32(&data, (uint32_t)formatData[format].size());
        data += formatData[format];
      }
    }
    clipboard->close();
  }

  return data;
}

bool IClipboard::copy(IClipboard *dst, const IClipboard *src)
{
  assert(dst != nullptr);
  assert(src != nullptr);

  return copy(dst, src, src->getTime());
}

bool IClipboard::copy(IClipboard *dst, const IClipboard *src, Time time)
{
  assert(dst != nullptr);
  assert(src != nullptr);

  bool success = false;
  if (src->open(time)) {
    if (dst->open(time)) {
      if (dst->empty()) {
        for (int32_t format = 0; format != static_cast<int>(Format::TotalFormats); ++format) {
          auto eFormat = (IClipboard::Format)format;
          if (src->has(eFormat)) {
            dst->add(eFormat, src->get(eFormat));
          }
        }
        success = true;
      }
      dst->close();
    }
    src->close();
  }

  return success;
}

uint32_t IClipboard::readUInt32(const char *buf)
{
  const auto *ubuf = reinterpret_cast<const unsigned char *>(buf);
  return (static_cast<uint32_t>(ubuf[0]) << 24) | (static_cast<uint32_t>(ubuf[1]) << 16) |
         (static_cast<uint32_t>(ubuf[2]) << 8) | static_cast<uint32_t>(ubuf[3]);
}

void IClipboard::writeUInt32(std::string *buf, uint32_t v)
{
  *buf += static_cast<uint8_t>((v >> 24) & 0xff);
  *buf += static_cast<uint8_t>((v >> 16) & 0xff);
  *buf += static_cast<uint8_t>((v >> 8) & 0xff);
  *buf += static_cast<uint8_t>(v & 0xff);
}
