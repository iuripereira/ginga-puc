/*
   Base64.cpp

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must
   not
      claim that you wrote the original source code. If you use this source
   code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not
   be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source
   distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#include "ginga.h"
#include "Base64.h"

GINGA_PRAGMA_DIAG_IGNORE (-Wconversion)

GINGA_UTIL_BEGIN

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

static inline bool
is_base64 (unsigned char c)
{
  return (isalnum (c) || (c == '+') || (c == '/'));
}

string
getBase64FromFile (const string &file_path)
{
  string sResult;
  FILE *readfile;
  char *buffer;
  unsigned long fileLen;

  readfile = fopen (file_path.c_str (), "rb");
  if (!readfile)
    {
      clog << "getBase64FromFile Unable to open file '" << file_path;
      clog << "'" << endl;
      return "";
    }

  struct stat stbuf;
  stat (file_path.c_str (), &stbuf);
  fileLen = stbuf.st_size;

  clog << "getBase64FromFile FILE LEN = " << fileLen << endl;
  // Allocate memory
  buffer = (char *)malloc (fileLen + 1);

  if (!buffer)
    {
      clog << "getBase64FromFile malloc" << fileLen << endl;
      fclose (readfile);
      return "";
    }

  // Read file contents into buffer
  int read_bytes = (int) fread (buffer, 1, stbuf.st_size, readfile);

  // dump_buffer(buffer, euli);

  fclose (readfile);

  // readFile

  sResult = base64_encode (reinterpret_cast<const unsigned char *> (buffer),
                           read_bytes);

  free (buffer);

  clog << "getBase64FromFile BASE 64 LEN = ";
  clog << sResult.length () << endl;

  // Finishes the base64 encoding and returns the result as a string
  return sResult;
}

/* writes a file to a path with data coming from a Base64 string */
int
writeFileFromBase64 (const string &payload, char *file_path)
{
  int len = -1;
  string result;

  ofstream out (file_path, ios::out | ios::binary);

  if (!out)
    {
      clog << "writeFileFromBase64 error opening file: ";
      clog << file_path << endl;
      return -1;
    }

  clog << "util::wb64 PAYLOAD SIZE = " << strlen (payload.c_str ()) << endl;

  string strDecode = base64_decode (payload);

  out << strDecode;
  out.close ();

  len = (int)payload.size ();
  /*
                  printf("util::functions::wb64 wrote %s to disk (from %d
     chars)\n",
                                  file_path,
                                  len);
  */
  clog << "util::wb64 wrote " << file_path;
  clog << " from " << len << " chars (b64)" << endl;
  return len;
}

std::string
base64_encode (unsigned char const *bytes_to_encode, unsigned int in_len)
{
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--)
    {
      char_array_3[i++] = *(bytes_to_encode++);
      if (i == 3)
        {
          char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
          char_array_4[1] = ((char_array_3[0] & 0x03) << 4)
                            + ((char_array_3[1] & 0xf0) >> 4);
          char_array_4[2] = ((char_array_3[1] & 0x0f) << 2)
                            + ((char_array_3[2] & 0xc0) >> 6);
          char_array_4[3] = char_array_3[2] & 0x3f;

          for (i = 0; (i < 4); i++)
            ret += base64_chars[char_array_4[i]];
          i = 0;
        }
    }

  if (i)
    {
      for (j = i; j < 3; j++)
        char_array_3[j] = '\0';

      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4)
                        + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2)
                        + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (j = 0; (j < i + 1); j++)
        ret += base64_chars[char_array_4[j]];

      while ((i++ < 3))
        ret += '=';
    }

  return ret;
}

std::string
base64_decode (const string &encoded_string)
{
  int in_len = encoded_string.size ();

  // std::clog << "::
  // base64_decode.encoded_string.size()="<<in_len<<std::endl;

  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && (encoded_string[in_] != '=')
         && is_base64 (encoded_string[in_]))
    {
      char_array_4[i++] = encoded_string[in_];
      in_++;
      if (i == 4)
        {
          for (i = 0; i < 4; i++)
            char_array_4[i] = base64_chars.find (char_array_4[i]);

          char_array_3[0]
              = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
          char_array_3[1] = ((char_array_4[1] & 0xf) << 4)
                            + ((char_array_4[2] & 0x3c) >> 2);
          char_array_3[2]
              = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

          for (i = 0; (i < 3); i++)
            ret += char_array_3[i];
          i = 0;
        }
    }

  if (i)
    {
      for (j = i; j < 4; j++)
        char_array_4[j] = 0;

      for (j = 0; j < 4; j++)
        char_array_4[j] = base64_chars.find (char_array_4[j]);

      char_array_3[0]
          = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4)
                        + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (j = 0; (j < i - 1); j++)
        ret += char_array_3[j];
    }

  return ret;
}

GINGA_UTIL_END
