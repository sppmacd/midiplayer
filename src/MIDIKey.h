#pragma once

#include <cstdint>

class MIDIKey
{
public:
    MIDIKey(uint8_t code)
    : m_code(code) {}

    bool is_black() const;
    float to_piano_position() const;

    operator uint8_t() const { return m_code; }
    MIDIKey operator-(int v) const { return MIDIKey{static_cast<uint8_t>(m_code - v)}; }

private:
    uint8_t m_code {};
};
