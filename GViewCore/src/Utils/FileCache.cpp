#include "GView.hpp"

using namespace GView::Utils;

constexpr unsigned int MAX_CACHE_SIZE = 0x1000000U; // 16 M

FileCache::FileCache()
{
    this->fileObj    = nullptr;
    this->cache      = nullptr;
    this->cacheSize  = 0;
    this->start      = 0;
    this->end        = 0;
    this->fileSize   = 0;
    this->currentPos = 0;
}
FileCache::~FileCache()
{
    if (this->fileObj)
    {
        this->fileObj->Close();
        delete this->fileObj;
    }
    this->fileObj = nullptr;
    if (this->cache)
        delete[] this->cache;
    this->cache = nullptr;
}

bool FileCache::Init(std::unique_ptr<AppCUI::OS::IFile> file, unsigned int _cacheSize)
{
    CHECK(this->cacheSize == 0, false, "Cache object already initialized !");
    this->fileObj = file.release(); // take ownership of the pointer
    CHECK(this->fileObj, false, "Expecting a valid file object poiner !");
    _cacheSize = (_cacheSize | 0xFFFF) + 1; // a minimum of 64 K for cache
    if (_cacheSize == 0)
        _cacheSize = MAX_CACHE_SIZE;
    _cacheSize     = std::min(_cacheSize, MAX_CACHE_SIZE);
    this->fileSize = fileObj->GetSize();

    this->cache = new unsigned char[_cacheSize];
    CHECK(this->cache, false, "Fail to allocate: %u bytes", _cacheSize);
    this->cacheSize = _cacheSize;
    this->start     = 0;
    this->end       = 0;

    return true;
}
BufferView FileCache::Get(unsigned long long offset, unsigned int requestedSize)
{
    CHECK(this->fileObj, BufferView(), "File was not properly initialized !");
    CHECK(requestedSize > 0, BufferView(), "'requestedSize' has to be bigger than 0 ");

    if (offset >= this->start)
    {
        // data is cached --> return from here
        if ((offset + requestedSize) <= this->end)
        {
            this->currentPos = offset + requestedSize;
            return BufferView(&this->cache[offset - this->start], requestedSize);
        }
        if (this->end == this->fileSize)
        {
            this->currentPos = this->fileSize;
            return BufferView(&this->cache[offset - this->start], (unsigned int) (this->end - offset));
        }
    }
    // request outside file
    if (offset >= this->fileSize)
        return Buffer();
    // data is not available in cache ==> read it
    unsigned long long _start, _end;
    if (this->fileSize <= this->cacheSize)
    {
        // read everything
        _start = 0;
        _end   = this->fileSize;
    }
    else
    {
        // compute the new buffer to read
        auto sz = requestedSize;
        if ((offset + sz) > this->fileSize)
            sz = (unsigned int) (this->fileSize - offset);
        if (sz > this->cacheSize)
            sz = this->cacheSize;
        auto diff = this->cacheSize - sz;
        if (diff <= offset)
            _start = offset - diff;
        else
            _start = 0;
        _end = _start + this->cacheSize;
        if (_end > this->fileSize)
            _end = this->fileSize;
    }
    // read new data in cache
    if (this->fileObj->SetCurrentPos(_start) == false)
        return BufferView();
    if (this->fileObj->Read(this->cache, (unsigned int) (_end - _start)) == false)
    {
        this->start = 0;
        this->end   = 0;
        return BufferView();
    }
    // return new pointer
    this->start = _start;
    this->end   = _end;
    if ((offset + requestedSize) <= this->end)
    {
        this->currentPos = offset + requestedSize;
        return BufferView(&this->cache[offset - this->start], requestedSize);
    }
    if (this->end == this->fileSize)
    {
        this->currentPos = this->fileSize;
        return BufferView(&this->cache[offset - this->start], (unsigned int) (this->end - offset));
    }
    this->currentPos = this->end;
    return BufferView(&this->cache[offset - this->start], (unsigned int) (this->end - offset));
}
bool FileCache::Copy(void* buffer, unsigned long long offset, unsigned int requestedSize)
{
    CHECK(buffer, false, "Expecting a valid pointer for a buffer !");
    auto b = Get(offset, requestedSize);
    CHECK(b.IsValid(), false, "Unable to read %u bytes from %llu offset ", requestedSize, offset);
    CHECK(b.GetLength() == requestedSize,
          false,
          "Unable to read %u bytes from %llu offset (only %u were read)",
          requestedSize,
          offset,
          (unsigned int)b.GetLength());
    memcpy(buffer, b.GetData(), b.GetLength());
    return true;
}
Buffer FileCache::CopyToBuffer(unsigned long long offset, unsigned int requestedSize)
{
    Buffer b(requestedSize);
    CHECK(Copy(b.GetData(), offset, requestedSize),
          Buffer(),
          "Fail to copy data to buffer (offset = %llu, size = %u)",
          offset,
          requestedSize);
    return b;
}