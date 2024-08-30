// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#pragma once

#include <string>

class B64Encode final
{
public:

    B64Encode(const void *data, std::size_t datasize, const std::string &extra = std::string(""));
    ~B64Encode();

    const std::string &GetEncodedData() const;

private:

    std::string b64data;
};