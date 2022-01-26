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
    int m_watcher_fd {};
    int m_watcher_wd {};
};
