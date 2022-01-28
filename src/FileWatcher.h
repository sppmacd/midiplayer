#pragma once

#include <functional>
#include <iostream>

class FileWatcher
{
public:
    explicit FileWatcher(std::string const& path);
    ~FileWatcher();

    bool file_was_modified() const;

private:
    int m_watcher_fd {-1};
    int m_watcher_wd {-1};
};
