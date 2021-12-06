#include "MIDIKey.h"

bool MIDIKey::is_black() const
{
    int relative_key = m_code % 12;
    return (relative_key == 1 || relative_key == 3 || relative_key == 6 || relative_key == 8 || relative_key == 10);
}

float MIDIKey::to_piano_position() const
{
    int relative_key = m_code % 12;
    int octave = m_code / 12;
    if(is_black())
        return (*this - 1).to_piano_position() + 0.75f;
    return white_key_index();
}

int MIDIKey::white_key_index() const
{
    if(is_black())
        return -1;
    int relative_key = m_code % 12;
    int octave = m_code / 12;
    if(relative_key < 1)
        return relative_key + octave * 7;
    if(relative_key < 3)
        return relative_key - 1 + octave * 7;
    if(relative_key < 6)
        return relative_key - 2 + octave * 7;
    if(relative_key < 8)
        return relative_key - 3 + octave * 7;
    if(relative_key < 10)
        return relative_key - 4 + octave * 7;
    return relative_key - 5 + octave * 7;
}

int MIDIKey::black_key_index() const
{
    if(!is_black())
        return -1;
    int relative_key = m_code % 12;
    int octave = m_code / 12;
    if(relative_key == 1)
        return octave * 5;
    if(relative_key == 3)
        return octave * 5 + 1;
    if(relative_key == 6)
        return octave * 5 + 2;
    if(relative_key == 8)
        return octave * 5 + 3;
    if(relative_key == 10)
        return octave * 5 + 4;
    return -1;
}
