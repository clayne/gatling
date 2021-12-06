#ifndef IMGIO_ERROR_CODES_H
#define IMGIO_ERROR_CODES_H

enum ImgioErrNo
{
  IMGIO_OK = 0,
  IMGIO_ERR_FILE_NOT_FOUND,
  IMGIO_ERR_IO,
  IMGIO_ERR_UNSUPPORTED_ENCODING,
  IMGIO_ERR_DECODE,
  IMGIO_ERR_UNSUPPORTED_FORMAT,
  IMGIO_ERR_UNKNOWN
};

#endif