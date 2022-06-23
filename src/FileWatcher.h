#pragma once

#include <functional>
#include <iostream>

class FileWatcher {
public:
    FileWatcher() = default;
    explicit FileWatcher(std::string const& path);
    FileWatcher(FileWatcher const&) = delete;
    FileWatcher& operator=(FileWatcher const&) = delete;
    FileWatcher(FileWatcher&&);
    FileWatcher& operator=(FileWatcher&&);
    ~FileWatcher();

    bool file_was_modified() const;

private:
    int m_watcher_fd { -1 };
    int m_watcher_wd { -1 };
};
