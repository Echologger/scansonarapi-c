// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#pragma once

#include <cstdint>

uint32_t Crc32_ComputeBuf(uint32_t inCrc32, const void *buf, std::size_t bufLen);
