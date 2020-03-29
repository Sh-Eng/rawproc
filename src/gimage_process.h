#ifndef __GIMAGE_PROCESS_H__
#define __GIMAGE_PROCESS_H__

#include <string>
#include "gimage/gimage.h"

std::map<std::string,std::string> process_blackwhitepoint(gImage &dib, std::map<std::string,std::string> params);
std::map<std::string,std::string> process_colorspace(gImage &dib, std::map<std::string,std::string> params);
std::map<std::string,std::string> process_crop(gImage &dib, std::map<std::string,std::string> params);

#endif
