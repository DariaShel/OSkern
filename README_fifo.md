# неблокирующий FIFO для JOS  

* в inc/fd.h, lib/fd.c добавляем новое устройство в `devtab` и статистику в `Stat`.
(изменение в `Stat` потребует небольшого обновления devfile_stat() в file.c):
```
static struct Dev *devtab[] = {&devfile,
                               &devpipe,
                               &devcons,
                               &devfifo,
                               0};
...

struct Stat {
  char st_name[MAXNAMELEN];
  off_t st_size;
  int st_isdir;
  int st_isfifo;
  struct Dev *st_dev;
};
```

* в fs/fs.h и fs/fs.c добавляем функцию `fifo_create`, аналогичную фукции `file_create`, 
но в поле `f->f_type` новая функция возвращает `FTYPE_FIFO`

* в inc/fs.h добавляем  
 -- структуру `Fifo`, имеющую поля с информацией о кол-ве читающих и пишущих процессов, 
 сдвиге на чтение и на запись и размере буффера  
 -- тип фаила `#define FTYPE_FIFO	2`  
 -- типы запросов к фаиловой системе
  `FSREQ_CREATE_FIFO, FSREQ_READ_FIFO, FSREQ_WRITE_FIFO, FSREQ_STAT_FIFO, FSREQ_CLOSE_FIFO`  
 -- в `Fsipc` добавлям структуры 
 `Fsreq_create_fifo, Fsreq_read_fifo, Fsreq_write_fifo, Fsreq_stat_fifo, Fsreq_close_fifo`  

 * inc/error.h добавляем коды ошибок
 ```
  E_FIFO = 20,	/* Fifo wait */
	E_FIFO_CLOSE = 21,	/* No readers or writers of Fifo */
 ```

 * в serve.c 
 -- небольшие изменения в `serve_open()` и `serve_stat()`  
 -- добавляем функции `serve_create_fifo, serve_read_fifo, serve_write_fifo, serve_stat_fifo, serve_close_fifo`  
 -- добавляем запросы fifo в handler в конце fs/serv.c  

 * прописываем логику FIFO в lib/fifo.c (код поход на file.c)

 * пишем пользовательскую программу, создающую FIFO в user/mkfifo.c

 * в fs/Makefrag добавляем USERAPPS `$(OBJDIR)/user/testfifo, $(OBJDIR)/user/mkfifo`

 * в kerm/Makefrag добавляем KERN_BINFILES `user/testfifo`

 * в lim/Marefrag добавляем LIB_SRCFILES `lib/fifo.c`

 * в sh.c добавлен обработчки для запуска приложений в фоне (&)


 ## тестирование FIFO
 1. 
 ```
 $ mkfifo my_fifo
 $ echo 123 > my_fifo
 $ echo 456 > my_fifo
 $ cat my_fifo
 >>> 123\n456
 $ cat my_fifo
 >>> <ничто>
```
 2. `make grade`  (с кофигурацией lab 13, нужно переименовать g_rade-lab13 в grade-lab13)
