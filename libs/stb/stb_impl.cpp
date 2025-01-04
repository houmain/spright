
#include <string>
#include <cstdint>
#include <cstring>
#include <new>

#if defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wconversion"
# pragma GCC diagnostic ignored "-Wsign-conversion"
# pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

static void* my_stbi_malloc(size_t size) {
  return new std::byte[size];
}

static void* my_stbi_realloc_sized(void* pointer, size_t old_size, size_t new_size) {
  auto old_data = static_cast<std::byte*>(pointer);
  if (old_size >= new_size)
    return old_data;

  auto new_data = new (std::nothrow) std::byte[new_size];
  if (new_data && old_data)
    std::memcpy(new_data, old_data, old_size);
  delete[] old_data;
  return new_data;
}

static void my_stbi_free(void* pointer) {
  auto data = static_cast<std::byte*>(pointer);
  delete[] data;
}

#define STBI_MALLOC(sz)                   my_stbi_malloc(sz)
#define STBI_REALLOC_SIZED(p,oldsz,newsz) my_stbi_realloc_sized(p,oldsz,newsz)
#define STBI_FREE(p)                      my_stbi_free(p)

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_JPEG
#define STBI_NO_PSD
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM 
#include "stb_image.h"


// set miniz compress function for stb PNG writer
// source: https://blog.gibson.sh/2015/07/18/comparing-png-compression-ratios-of-stb_image_write-lodepng-miniz-and-libpng/
#include "miniz/miniz.h"

static unsigned char*
my_stbi_zlib_compress( unsigned char *data, int data_len,
                       int *out_len, int quality )
{
  mz_ulong buflen = mz_compressBound(data_len);
  // Note that the returned buffer will be free'd by stbi_write_png*()
  // with STBIW_FREE(), so if you have overridden that (+ STBIW_MALLOC()),
  // adjust the next malloc() call accordingly:
  unsigned char* buf = static_cast<unsigned char*>(malloc(buflen));
  if( buf == NULL
      || mz_compress2(buf, &buflen, data, data_len, quality) != 0 )
  {
      free(buf); // .. yes, this would have to be adjusted as well.
      return NULL;
  }
  *out_len = buflen;
  return buf;
}
#define STBIW_ZLIB_COMPRESS  my_stbi_zlib_compress

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
