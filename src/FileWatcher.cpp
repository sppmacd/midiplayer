#include "FileWatcher.h"

#include <cstring>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/inotify.h>
#include <unistd.h>

FileWatcher::FileWatcher(std::string const& path)
{
    m_watcher_fd = inotify_init();
    if(m_watcher_fd < 0)
    {
        std::cerr << "WARNING: Failed to setup watcher: " << strerror(errno) << std::endl;
        m_watcher_fd = -1;
        return;
    }

    if(fcntl(m_watcher_fd, F_SETFL, fcntl(m_watcher_fd, F_GETFL) | O_NONBLOCK) < 0)
    {
        std::cerr << "WARNING: Failed to set watcher non-blocking: " << strerror(errno) << std::endl;
        m_watcher_fd = -1;
        return;
    }

    m_watcher_wd = inotify_add_watch(m_watcher_fd, path.c_str(), IN_MODIFY);
    if(m_watcher_wd < 0)
    {
        std::cerr << "WARNING: Failed to setup watch for " << path << ": " << strerror(errno) << std::endl;
        m_watcher_fd = -1;
        return;
    }
}

FileWatcher::FileWatcher(FileWatcher&& other)
{
    *this = std::move(other);
}

FileWatcher& FileWatcher::operator=(FileWatcher&& other)
{
    if(this == &other)
        return *this;

    m_watcher_fd = std::exchange(other.m_watcher_fd, -1);
    m_watcher_wd = std::exchange(other.m_watcher_wd, -1);
    return *this;
}

bool FileWatcher::file_was_modified() const
{
    if(m_watcher_fd == -1)
        return false;
    std::vector<char> buffer;
    buffer.resize(sizeof(inotify_event) + NAME_MAX + 1);
    if(read(m_watcher_fd, buffer.data(), buffer.size()) < 0)
    {
        if(errno != EAGAIN)
            std::cerr << "WARNING: Failed to read inotify event: " << strerror(errno) << std::endl;
        return false;
    }
    inotify_event* event = (inotify_event*)buffer.data();
    return event->wd == m_watcher_wd;
}

FileWatcher::~FileWatcher()
{
    inotify_rm_watch(m_watcher_fd, m_watcher_wd);
}
