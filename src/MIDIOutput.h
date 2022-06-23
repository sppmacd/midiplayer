#pragma once

class Event;

class MIDIOutput {
public:
    virtual ~MIDIOutput() = default;
    virtual bool is_valid() const = 0;
    virtual void write_event(Event const&) = 0;
};
