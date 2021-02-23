struct fd_t {
	int fd = -1;
	int err = 0;
	fd_t(const char *path, int flags = O_RDONLY) {
		fd = open(path, flags);
		if (fd == -1) {
			err = errno;
		}
	}
	fd_t(const fd_t &) = delete;
	~fd_t() {
		if (fd != -1) {
			close(fd);
		}
	}
	operator bool() const {
		return fd != -1;
	}
	operator int() const {
		return fd;
	}
};
