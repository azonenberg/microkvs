# Introduction

microkvs is a log structured key-value store for embedded devices intended for storage of configuration data,
encryption keys, and other relatively small information that needs to be persisted across power cycles. An object
oriented driver layer provides access to the underlying storage media, typically either unused space in code flash or
an external SPI NOR flash. This enables microkvs to be easily ported to new platforms in the future.

There are no dependencies on dynamic RAM allocation or any fancy libc APIs that might be unavailable in an embedded
platform.

This is a work in progress and subject to change. Until a formal release, there may be breaking changes to the flash
data structure at any time.

# General concepts

From a programmer's perspective, microkvs provides access to a series of objects (arbitrary blobs of binary data),
identified by keys (16-character ASCII strings, analogous to file names although no hierarchy is supported).

Objects are immutable and copy-on-write due to the nature of the underlying raw flash storage, which defaults to all 1s
in the erased state and can only be written to logic 0. Writes typically support fine granularity (down to byte level)
however erases have very large granularity (4 to 256 kB is common).

microkvs does not support extent based / fragmented object storage. All objects are stored contiguously in the media;
any change to an object requires completely rewriting it. If portions of an object need to be changed frequently while
others do not, consider breaking it up into several objects to reduce write amplification effects.

If memory mapping is supported by the underlying storage, objects can be directly memory mapped for read-only access.
Memory mapped writing is not supported due to hardware limitations.

# Architecture details

The backing store for microkvs consists of two equally sized "banks" of flash memory in separate erase blocks. Microkvs
treats each bank as one erase block at a logical level even if it physically consists of multiple blocks which must be
erased in sequence, since the typical use case is to store a small amount of configuration data which easily fits in a
single flash block.

microkvs prefers byte writable storage. It can function with storage that has larger write block granularity (for
example STM32H7 internal flash memory, with a 256 bit / 32 byte write block size) however overhead is increased as all
data structures must be padded to multiples of a write block. Byte writable storage is assumed by default; to enable
padding set a global preprocessor definition MICROKVS_WRITE_BLOCK_SIZE to the desired block size.

The store is divided into two regions, log and data. The split must be decided at compile time and cannot be changed
later on. The optimal split is application dependent and varies based on average file size weighted by how often each
file is modified. A minimum of 28 bytes of storage are required in the log area for each object stored in the data
area, unless padded by minimum write block sizes.

For example, with 256K byte storage and 228 byte average file size, a good split would be 28K bytes of log and 228K
bytes of data. This would allow roughly 1024 objects worth of both log and payload to be stored before both areas are
exhausted simultaneously and a garbage collection is required.

## Locating the active bank

Each bank has a static header at the start of the log area containing a 32-bit version number. Version numbers start at
0 and increment; the special value 0xffffffff is used for marking an invalid/blank block. No provision is made for
handling overflow as typical flash memory has far less than 2^32 erase cycles of endurance, so the media will wear out
before the counter wraps.

The bank with the highest non-0xffffffff version number is active.

## Locating an object

To locate an object, the log in the active bank is scanned from start to end searching for entries with the requested
key. The last matching entry in the log points to the current version of the object.

The CRC-32 checksum of the object is verified before the location is returned. If the checksum fails, earlier versions
of the object (if present) are scanned, and the most recent version with a valid checksum is returned. If no copy with
a valid checksum can be located, an error is returned.

## Writing an object

To write an object, the start address and length of the last valid log entry are used to calculate the location of the
first free byte in the data area. If insufficient space is present, or no free log space is available, a garbage
collection must be performed.

Once the space is confirmed available, a new log entry is created and the start address and length are written. The key
and CRC are left blank since the data is not yet present (the previous version of the object is current), however the
space is now reserved and cannot be used by another object.

After reading back the log entry to confirm a correct write, the object content is written and the CRC is calculated.
The object content is immediately read back to confirm a correct write; if an error is seen the log entry is left with
a blank key and CRC (space reserved but data ignored) and the write is attempted again to a new location.

Once all object data has been successfully written, the CRC and name in the log entry are written to commit the object.

## Garbage collection

To garbage collect, the inactive block is erased. The latest entry of each object is then written to the new block, and
after verification the block header is written with a new revision number one higher than the current.

# Flash storage format

## Bank header

```
uint32_t magic = 0xc0def00d
uint32_t version
uint32_t logSize
```

## Log entry

```
char     key[16]
uint32_t start
uint32_t len
uint32_t crc
```

## Data area

Data objects consist of raw binary data with a CRC-32 checksum (using the common Ethernet/ZIP polynomial 0x04c11db7)
appended.
