#pragma once

#include "Property.h"
#include "Time.h"

namespace Config {

class Reader;

class Condition {
public:
    virtual ~Condition() = default;
    virtual bool is_met(Reader& reader) const = 0;
    virtual bool has_expired(Reader& reader) const { return false; }
};

class StartupCondition : public Condition {
public:
    virtual bool is_met(Reader& reader) const override;
    virtual bool has_expired(Reader& reader) const override;
};

class TimeCondition : public Condition {
public:
    explicit TimeCondition(Time time)
        : m_time(time)
    {
    }

    virtual bool is_met(Reader& reader) const override;
    virtual bool has_expired(Reader& reader) const override;

private:
    Time m_time;
};

class ModeCondition : public Condition {
public:
    // FIXME: This should actually be in MIDIPlayer
    enum class Mode {
        Realtime,
        Play
    };

    explicit ModeCondition(Mode mode)
        : m_mode(mode)
    {
    }

    virtual bool is_met(Reader& reader) const override;
    virtual bool has_expired(Reader& reader) const override;

private:
    Mode m_mode {};
};

}
