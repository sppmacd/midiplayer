#pragma once

class Event;

class MIDIOutput
{
public:
    virtual ~MIDIOutput() = default;
    virtual void write_event(Event const&) = 0;
};
