#include <inc/fs.h>
#include <inc/string.h>
#include <inc/lib.h>

union Fsipc fsipcbuf_fifo __attribute__((aligned(PAGE_SIZE)));

static int
fsipc_fifo(unsigned type, void *dstva)
{
	static envid_t fsenv;
	if (fsenv == 0) fsenv = ipc_find_env(ENV_TYPE_FS);

	// static_assert(sizeof(fsipcbuf_fifo) == PAGE_SIZE, "Invalid fsipcbuf size");

	if (debug)
		cprintf("[%08x] fsipc %d %08x\n", thisenv->env_id, type, *(uint32_t *)&fsipcbuf_fifo);

	ipc_send(fsenv, type, &fsipcbuf_fifo, PAGE_SIZE, PROT_RW);
	size_t maxsz = PAGE_SIZE;
    return ipc_recv(NULL, dstva, &maxsz, NULL);
}

static int devfifo_close(struct Fd *fd);
static ssize_t devfifo_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devfifo_write(struct Fd *fd, const void *buf, size_t n);
static int devfifo_stat(struct Fd *fd, struct Stat *stat);
static int devfile_trunc(struct Fd *fd, off_t newsize);


struct Dev devfifo =
{
	.dev_id = 'o',
	.dev_name = "fifo",
	.dev_read = devfifo_read,
	.dev_close = devfifo_close,
	.dev_stat = devfifo_stat,
	.dev_write = devfifo_write,
	.dev_trunc = devfile_trunc,
};

int
mkfifo(const char *path)
{
	int res;

	if (strlen(path) >= MAXPATHLEN) return -E_BAD_PATH;

	strcpy(fsipcbuf_fifo.create_fifo.req_path, path);

	if ((res = fsipc_fifo(FSREQ_CREATE_FIFO, NULL)) < 0) return res;

	return 0;
}

static int
devfifo_close(struct Fd *fd)
{
	fsipcbuf_fifo.close_fifo.req_fileid = fd->fd_file.id;
	return fsipc_fifo(FSREQ_CLOSE_FIFO, NULL);
}

static ssize_t
devfifo_read(struct Fd *fd, void *buf, size_t n)
{
	int r;
	int res = 0;

	while(1){

		fsipcbuf_fifo.read_fifo.req_fileid = fd->fd_file.id;
		fsipcbuf_fifo.read_fifo.req_n = n - res;

		if ((r = fsipc_fifo(FSREQ_READ_FIFO, NULL)) < 0){
			if (r == -E_FIFO){
				memmove(buf + res, fsipcbuf_fifo.readRet.ret_buf, fsipcbuf_fifo.readRet.ret_n);
				res += fsipcbuf_fifo.readRet.ret_n;
				if (debug)
					cprintf("read wait\n");
				sys_yield();
				continue;
			} else if (r == -E_FIFO_CLOSE) {
                // нет писателей, возвращаем все, что было в канале
                memmove(buf + res, fsipcbuf_fifo.readRet.ret_buf, fsipcbuf_fifo.readRet.ret_n);
				return res + fsipcbuf_fifo.readRet.ret_n;
			}

			return r;
		}

		memmove(buf + res, fsipcbuf_fifo.readRet.ret_buf, r);
		res += r;
		break;
	}

	return res;
}

static ssize_t
devfifo_write(struct Fd *fd, const void *buf, size_t n)
{
	int r;
	int res = 0;

	int buf_size = sizeof(fsipcbuf_fifo.write_fifo.req_buf);

	while(1) {

		fsipcbuf_fifo.write_fifo.req_fileid = fd->fd_file.id;
		fsipcbuf_fifo.write_fifo.req_n = MIN((n - res), buf_size);
		memmove(fsipcbuf_fifo.write_fifo.req_buf, 
		        buf + res, 
				MIN((n - res), buf_size));
		
		if ((r = fsipc_fifo(FSREQ_WRITE_FIFO, NULL)) < 0){
			if (r == -E_FIFO){
				res += fsipcbuf_fifo.writeRet.ret_n;
				if (debug)
					cprintf("write \n");
				sys_yield();
				continue;
			} else if (r == -E_FIFO_CLOSE) {
                // нет читателей
                cprintf("devfifo_write() sends SIGPIPE -- there is no readers in this fifo. \n");
                sigqueue(sys_getenvid(), SIGPIPE, (const union sigval)0);
                return res;
			}

			return r;
		}

		res += r;
		if (res >= n){
			break;
		}
	}

	return res;
}

static int
devfifo_stat(struct Fd *fd, struct Stat *st)
{
	int r;

	fsipcbuf_fifo.stat_fifo.req_fileid = fd->fd_file.id;
	if ((r = fsipc_fifo(FSREQ_STAT_FIFO, NULL)) < 0)
		return r;
	strcpy(st->st_name, fsipcbuf_fifo.statRet.ret_name);
	st->st_size = fsipcbuf_fifo.statRet.ret_size;
	st->st_isdir = fsipcbuf_fifo.statRet.ret_isdir;
	st->st_isfifo = fsipcbuf_fifo.statRet.ret_isfifo;
	return 0;
}

// this function does nothing, but it usefull 
// for compatobiliy with file's interfaces
static int
devfile_trunc(struct Fd *fd, off_t newsize) {
    return 0;
}