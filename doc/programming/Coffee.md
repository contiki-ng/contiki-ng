# Coffee

Coffee is a minimalistic, yet fully functional file system that operates with the peculiar characteristics of flash memories and EEPROM [[Tsiftes et al. '09](http://ieeexplore.ieee.org/abstract/document/5211918/)]. One of the design principles of Coffee is that its implementation size must be very small in order to be enabled by default in sensor devices running Contiki-NG. To fulfill this goal, Coffee is designed to make the file structure simple by using extents, leading to a significantly reduced need for storing metadata in RAM.

Flash memory make file modifications more complicated to handle than magnetic disks do. The common trait of flash memories is that bits can be toggled from 1 to 0, but not toggled back from 0 to 1 without doing an expensive erase operation on a large number of bits. To accommodate file modifications, Coffee introduces a file structure called a micro log. When file data is first about to be overwritten, Coffee creates a new invisible file that is linked to the original file. The invisible file is a small log structure within a regular file, belonging to the original file and containing the most recently written data in the logical file. Although this concept is similar to log structuring [[Rosenblum & Ousterhout '91](https://dl.acm.org/citation.cfm?id=121137)], a popular technique in flash file systems, Coffee's micro logs differs from log structuring because it requires very little metadata in RAM, and allows optimization on a per-file basis. When the micro log eventually fills up, Coffee transparently merges the content of the original file and the micro log into a new file, and deletes the two former files.

Coffee can be programmed by using the CFS Programming Interface, along with some Coffee-specific extensions. We describe the details of the programming interface below, as well as explain some special characteristics of Coffee that programmers should take into account.

## The CFS Programming Interface

Applications access file systems by using the Contiki-NG File System (CFS) API. Each file system implements basic functions for reading and writing files, and extracting directory contents in a manner similar to the POSIX file API. The API is intentionally simple to keep the implementation size small, but the functionality covers the most common
uses of a file system. The table below lists the available functions, which are declared in `os/storage/cfs/cfs.h`.


| Function                                                          | Purpose                             |
|------------------------------------------------------------------ |-------------------------------------|
|`int cfs_open(const char *name, int flags)`                        | Open a file.                        |
|`void cfs_close(int fd)`                                           | Close an open file.                 |
|`int cfs_read(int fd, void *buf, unsigned int len)`                | Read data from an open file.        |
|`int cfs_write(int fd, const void *buf, unsigned int len)`         | Write data to an open file.         |
|`cfs_offset_t cfs_seek(int fd, cfs_offset_t offset, int whence)`   | Move to a position in an open file. |
|`int cfs_remove(const char *name)`                                 | Remove a file.                      |
|`int cfs_opendir(struct cfs_dir *dirp, const char *name)`          | Open a directory.                   |
|`int cfs_readdir(struct cfs_dir *dirp, struct cfs_dirent *dirent)` | Read a directory entry.             |
|`void cfs_closedir(struct cfs_dir *dirp)`                          | Close an open directory.            |

### Opening and Closing Files

Every open file is represented by a unique file descriptor. The file descriptor is of type `int` and is provided as an argument to all CFS functions that deal with a file.

Files are opened by calling `cfs_open()` with a filename and a set of flags as arguments. The flags are `CFS_READ`, `CFS_WRITE`, and `CFS_APPEND`. Multiple flags can be set by using the logical OR operator. `CFS_READ` specifies that the file should be opened for reading, whereas `CFS_WRITE` enables file writing. The `CFS_WRITE` flag causes different actions depending on whether `CFS_APPEND` is set. If `CFS_WRITE` is set, but `CFS_APPEND` is not set, the file is truncated to a size of 0. If both `CFS_WRITE` and `CFS_APPEND`, then the file is preserved and the file offset is set to point at the byte offset immediately after the end of the file. Setting `CFS_APPEND` implicitly means that `CFS_WRITE` is set. `cfs_open()` returns -1 if the file could not be opened, or a value above or equal to 0 if the file was opened.

When an open file is no longer needed, the application should close it by using `cfs_close()`. By closing a file, the file system can deallocate its internal resources held for the file, and possibly commit any cached data to permanent storage.

Files can be removed in CFS-POSIX and Coffee by calling `cfs_remove()`, which takes the name of the file as a parameter. `cfs_remove()` returns 0 if the file was removed, or -1 if the file could not be removed or is nonexistent.

### Using Files

After opening a file and thereby obtaining a file descriptor for it, the file can be used according to the flags that the were specified to `cfs_open()`. `cfs_read()` fills `buf` with at most `len` bytes, starting from the current position in the file that is stored in the file descriptor. It returns the amount of bytes read, or -1 if an error occurs.

`cfs_write()` writes `len` bytes from the memory buffer `buf` into the file, starting from the current position in the file descriptor. The file must have been opened with the `CFS_WRITE` flag. It returns the amount of bytes written, or -1 if an error occurred.

Note that neither `cfs_write()` nor `cfs_read()` are guaranteed to write and read the full amount of bytes requested in one call.

`cfs_seek()` moves the current file position to the position determined by the combination of the `offset` and the `whence`. `CFS_SEEK_SET` tells `cfs_seek()` to compute the offset from the beginning of the file, i.e., as an absolute offset. `CFS_SEEK_CUR` specifies that the offset should be compute relative to the current position of the file position. Similarly, `CFS_SEEK_END` computes the offset in relation to the end of the file, and can be used to move beyond the end if the file system implementation allows it. Negative offset values are accepted by both `CFS_SEEK_CUR` and `CFS_SEEK_END`, if the program wishes to move the file position backwards from the base that is indicated by the `whence` parameter. `cfs_seek()` returns the new absolute file position upon success, or -1 if the file pointer could not be moved to the requested position.

Note that the length of the file can be retrieved by calling `cfs_seek()` with `CFS_SEEK_END` as the `whence` value, and 0 as the `offset`. The return value of `cfs_seek()` is the file length, or -1 if the operation failed or is not supported. An immediate `cfs_read()` from the position will not return a positive value.

In the following example, we show how Coffee can be used to write some data to a file and read the written data after moving the file position to the start.

```c
int fd;
char buf[] = "Hello, World!";

fd = cfs_open("test", CFS_READ | CFS_WRITE);
if(fd >= 0) {
  cfs_write(fd, buf, sizeof(buf));
  cfs_seek(fd, 0, CFS_SEEK_SET);
  cfs_read(fd, buf, sizeof(buf));
  printf("Read message: %s\n", buf);
  cfs_close(fd);
}
```

### Listing Directory Contents

Applications can retrieve directory contents by using a combination of three functions. `cfs_opendir()` opens the directory `name` and fills in an opaque handle pointed to by `dirp`. The contents of this handle is unspecified and is only for use internally by Coffee. If `cfs_opendir()` fails to open the directory, the function returns -1. Upon a successful directory opening, `cfs_opendir()` return 0. A directory that has been opened must be closed after being used by calling `cfs_closedir()` with the `dirp` handle supplied as an argument.

The directory contents can be read, one entry at a time, with the `cfs_readdir()` function. `cfs_readdir()` takes two arguments, the directory handle `dirp` and a `dirent` object. `cfs_readdir()` requires that the `dirent` object is preallocated by the function caller. `dirent` is of type `struct cfs_dirent`, which is defined below. It simply contains the name and the size of the file.

```c
struct cfs_dirent {
  char name[32];
  cfs_offset_t size;
};
```

If a directory entry can be retrieved by `cfs_readdir()`, it writes the directory entry into the space pointed to by `dirent` and returns 0. If no more entries were found, `cfs_readdir()` returns -1. In the following example, we show how the directory contents can be printed to standard output.

```c
struct cfs_dir dir;
struct cfs_dirent dirent;

if(cfs_opendir(&dir, "/") == 0) {
  while(cfs_readdir(&dir, &dirent) != -1) {
    printf("File: %s (%ld bytes)\n",
           dirent.name, (long)dirent.size);
  }
  cfs_closedir(&dir);
}
```

### Coffee extensions of CFS

Coffee extends the CFS API with three functions that are shown below. These functions are declared in
`os/storage/cfs/cfs-coffee.h`.

|Function                                                              | Purpose                                     |
|----------------------------------------------------------------------|---------------------------------------------|
|`int cfs_coffee_configure_log(const char *file, unsigned log_size, unsigned log_entry_size)` | Configure a micro log file. |
|`int cfs_coffee_format(void)`                                         | Format the storage area assigned to Coffee. |
|`int cfs_coffee_reserve(const char *name, cfs_offset_t size)`         | Reserve space for a file.                   |

`cfs_coffee_format()` must be called before using Coffee for the first time on a storage device. When the storage device is a flash memory, this operation is likely to run for several seconds because all sectors must be erased. The function returns 0 if it is successful, or -1 if it fails. The most plausible reason for failing is that Coffee is configured incorrectly.

`cfs_coffee_reserve()` preallocates an extent of the specified size. This function is not necessary for using Coffee, but it optimizes Coffee's handling of the file if the file size is known beforehand. If `cfs_coffee_reserve()` is not called before opening a file for the first time with `CFS_WRITE` set, Coffee creates an extent with a default size that is device dependent. The function returns 0 if it successfully allocated an extent, or -1 if it failed to do so.

The function to use for tuning a micro log is called `cfs_coffee_configure_log()`. Its two parameters determine how large the log should be (`log_size`), and how large each log entry should be (`log_entry_size`.) Finding the optimal values is a question of examing the I/O access pattern of the calling application before deploying it. If this function is not called, Coffee uses a default micro log size, as well as a default log entry size which is likely to match the page size of the storage device. Like `cfs_coffee_reserve()`, `cfs_coffee_configure_log()` must be called before the file has been created.

### Files

As we mentioned, Coffee files have the physical layout of an extent, possibly coupled with a micro log. Each ordinary file consists of a header and a data area, whereas each micro log file substitutes a log index table and a log entry table with the data area. Micro logs are handled transparently by Coffee so that programs are presented with the logical contents of a file directly through the `cfs_write()` and `cfs_read()` functions. All types of files in Coffee have a preset maximum size that is defined in a number of platform-defined pages.

When first opening a file after the system start, Coffee has to scan the storage device sequentially to find the right extent. Thus there is a slight warm-up delay for Coffee, but subsequent file openings will use a small internal cache of `struct file` objects, as shown in  the code listing below. File descriptors that point to the same file share a common `struct file` object, thereby ensuring consistency when the same file has been opened by multiple programs.

```c
/* The structure of cached file objects. */
struct file {
  cfs_offset_t end;
  coffee_page_t page;
  coffee_page_t max_pages;
  int16_t record_count;
  uint8_t references;
  uint8_t flags;
};

/* The file descriptor structure. */
struct file_desc {
  cfs_offset_t offset;
  struct file *file;
  uint8_t flags;
  uint8_t io_flags;
};
```

Another characteristic of Coffee that occurs when opening a file for the first time is that its end of file position must be found. Coffee does not store this data in the header, since the end of file position is often highly volatile, and flash devices do not allow repeated modifications in the same flash memory address. Coffee uses a brute force scan backwards from the end of the extent to find the first non-null byte. This induces a semantic consequence on files in which the last written byte was a 0: it will not be accounted for when reopening the file after a system start. Coffee will cache the end of file position in the `struct file` object though, which removes the problem in files that are cached and reopened. In order to avoid this problem, we recommend that the program ensures that the last written
byte is always non-null, or that the program appends the null values if it can determine that they are missing.

### Garbage Collection

Removing a file from a Coffee file system is a two-step process with the first step being initiated by the external user, and the second step being initiated automatically by Coffee. The first step is the ordinary call to `cfs_remove()` with the filename supplied as an argument. In most Coffee ports, this does not remove the file extent physically from the storage device&mdash;it just marks it as obsolete and thereby eligible for garbage collection. An obsolete file is invisible to external users, but occupies the same space as the file did when it was allocated.

Coffee initiates the garbage collection step when a new file reservation request cannot be granted. The garbage collector operates sequentially over the storage device, which is divided into an array of sectors. For each sector it checks if the sector contains at least one obsolete page and no active pages. If the check succeeds, Coffee erases the sector. There is a possibility that obsolete pages spans more sectors than the one being erased, but in that case Coffee splits the remaining pages into isolated pages that belong to no file. The isolated pages are treated in the same way as obsolete pages when they are processed by the garbage collector.

### The Root Directory

Coffee has a flat directory structure that is obtained implicitly by scanning for the ordinary file extents. When calling `cfs_opendir()` on the only directory, also known as the root directory, Coffee is accepts either "/" or "." as the directory name. In each iteration with `cfs_readdir()`, Coffee uses a quick skip algorithm that is able to jump over large spaces of free memory. The iterative process may take a longer time, however, if there are many small files&mdash;either allocated or marked as obsolete&mdash;in the file system.

### Porting Coffee

The Coffee implementation in `os/storage/cfs/cfs-coffee.c` is purely implementation-independent. It relies on a set of macro definitions to point to the platform-dependent configuration values and I/O functions. Each platform using Coffee has a `cfs-coffee-arch.h` that defines the Coffee macros, which are listed in the table below.

| Macro                          | Purpose                                                    |
|--------------------------------|------------------------------------------------------------|
|COFFEE_WRITE(buf, size, offset) | A platform-defined write function.                         |
|COFFEE_READ(buf, size, offset)  | A platform-defined read function.                          |
|COFFEE_ERASE(sector)            | A platform-defined erase function.                         |
|COFFEE_SECTOR_SIZE              | The logical sector size.                                   |
|COFFEE_PAGE_SIZE                | The logical page size.                                     |
|COFFEE_START                    | The start offset of the file system.                       |
|COFFEE_SIZE                     | The total size in bytes of the file system.                |
|COFFEE_NAME_LENGTH              | The maximum filename length.                               |
|COFFEE_MAX_OPEN_FILES           | The amount of file cache entries.                          |
|COFFEE_FD_SET_SIZE              | The amount of file descriptor entries.                     |
|COFFEE_LOG_TABLE_LIMIT          | The maximum amount of log table entries read in one batch. |
|COFFEE_DYN_SIZE                 | The default reserved file size.                            |
|COFFEE_LOG_SIZE                 | The default micro log size.                                |
|COFFEE_MICRO_LOGS               | Specify whether Coffee will use micro logs.                |

Porting Coffee is usually a simple task of mapping these configuration parameters to the storage device parameters, and to define `COFFEE_WRITE`, `COFFEE_READ`, and `COFFEE_ERASE` to point to the device drivers I/O functions. In cases where the API's do not much, for example when the device drivers provides only page-based I/O, a small emulation layer for random I/O is needed.

`COFFEE_PAGE_SIZE` denotes the logical page size. The data type to use for pages&mdash;`coffee_page_t`&mdash;must also be specified in `cfs-coffee-arch.h`. This data type must be able to hold `COFFEE_SIZE} / COFFEE_PAGE_SIZE` pages. Coffee further requires that `sizeof(COFFEE\_PAGE\_SIZE) >= sizeof(cfs_offset_t)`. The type of `COFFEE_PAGE_SIZE` can be controlled either by a type cast, or by a value suffix denoting the wanted type.

The parameters must not always match those of the device driver. For example, `COFFEE_SECTOR_SIZE` and `COFFEE_PAGE_SIZE` could be tuned for other values, as long as the I/O functions are aware of this. In one end of the spectrum, we have large storage devices, such as those SD cards, which may require a large logical `COFFEE_SECTOR_SIZE` in order to avoid slow sequential scans over the memory space. At the other end, we have small EEPROM devices that do not use the notion of pages. In this case we recommend that programmers define some small logical sector size and page size (i.e., larger than the Coffee header size) in order to obtain a reasonable performance from Coffee.

`COFFEE_START` is useful if some part in the beginning of the storage device should be used for other purposes. `COFFEE_SIZE` specifies how many bytes&mdash;starting from `COFFEE_START&mdash;that should be used by Coffee.

The `COFFEE_MAX_OPEN_FILES` and `COFFEE_FD_SET_SIZE` should be set to values that accommodate the largest expected working sets of the used platform. These parameters define the amount of entries in the arrays `struct file` and `struct file_descriptor`, respectively. Hence, these variables affect amount of static memory used by Coffee. Another
parameter that affects Coffee's RAM footprint is `COFFEE_LOG_TABLE_LIMIT`, which is used for reading batches of log table entries. Coffee can push up to `COFFEE_LOG_TABLE_LIMIT * 2` bytes onto the stack during micro log operations.

`COFFEE_DYN_SIZE` and `COFFEE_LOG_SIZE` determine the default size that Coffee allocates for ordinary files and micro logs. It is up to the port developer to define suitable values for the size of the storage device. This step may require fine-tuning in order to find the right balance between performance and low space overhead.

Lastly, if the `COFFEE_MICRO_LOG` parameter is set to 1, Coffee is compiled with all micro-log-related functions included. Otherwise if the value is set to 0, Coffee assumes that the storage device can handle in-place modifications, and does therefore exclude micro logs and ignores the parameters regarding micro logs. Alternatively, if a user knows that no written data in any file will be overwritten, the micro log functionality can be switched off for the purpose of reducing Coffee's code size considerably.
