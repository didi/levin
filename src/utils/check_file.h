#ifndef  LEVIN_CHECK_FILE_H
#define  LEVIN_CHECK_FILE_H

#include <string>

namespace levin {

bool CheckFileMD5(const std::string file_path, const std::string verify_data);

}

#endif  // LEVIN_CHECK_FILE_H

