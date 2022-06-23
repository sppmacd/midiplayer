#pragma once

#include <cstring>
#include <iostream>
#include <pthread.h>
#include <span>
#include <sstream>

// FIXME: Remove once C++23 std::ispanstream is available

class SpanStreamBuf : public std::streambuf {
public:
    SpanStreamBuf(std::span<uint8_t> data)
        : m_data(data)
    {
    }

private:
    // Positioning
    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode) override
    {
        if (pos > m_data.size())
            m_offset = m_data.size();
        else
            m_offset = pos;
        return m_offset;
    }

    // Get area
    virtual std::streamsize xsgetn(char_type* s, std::streamsize count) override
    {
        if (m_offset + count > m_data.size())
            count = m_data.size() - m_offset;
        memcpy(s, m_data.data() + m_offset, count);
        m_offset += count;
        return count;
    }

    virtual std::streamsize showmanyc() override
    {
        return m_data.size() - m_offset;
    }

    std::span<uint8_t> m_data;
    size_t m_offset {};
};

class ISpanStream : public std::istream {
public:
    ISpanStream(std::span<uint8_t> data)
        : m_buffer(data)
    {
        rdbuf(&m_buffer);
    }

private:
    SpanStreamBuf m_buffer;
};
