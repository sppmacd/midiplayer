#pragma once

class Pedals {
public:
    void set_soft(bool b) { m_soft = b; }
    bool soft() const { return m_soft; }

    void set_sostenuto(bool b) { m_sostenuto = b; }
    bool sostenuto() const { return m_sostenuto; }

    void set_sustain(bool b) { m_sustain = b; }
    bool sustain() const { return m_sustain; }

private:
    bool m_soft = false;
    bool m_sostenuto = false;
    bool m_sustain = false;
};
