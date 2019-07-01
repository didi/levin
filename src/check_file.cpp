#include "check_file.h"
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include "openssl/md5.h"
#include "levin_logger.h"

namespace levin {

bool GetFileMD5(const std::string& file_name, std::string& md5) {
    int fd = open(file_name.c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    }

    MD5_CTX    ctx;
    ssize_t    bytes;
    const uint32_t MAX_MD5_BUFF_SIZE = 4096;
    char       buff[MAX_MD5_BUFF_SIZE];
    unsigned char out[MD5_DIGEST_LENGTH];
    char sz_result[2 * MD5_DIGEST_LENGTH + 1];

    MD5_Init(&ctx);
    while ((bytes = read(fd, buff, MAX_MD5_BUFF_SIZE)) != 0) {
        MD5_Update(&ctx, buff, bytes);
    }
    MD5_Final(out, &ctx);

    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        sprintf(sz_result + i * 2, "%02x", out[i]); 
    }
    sz_result[2 * MD5_DIGEST_LENGTH] = '\0';
    md5 = sz_result;
    close(fd);

    return true;
}

bool CheckFileMD5(const std::string file_path, const std::string verify_data) {
    std::string md5;
    if (!GetFileMD5(file_path, md5)) {
        LEVIN_CWARNING_LOG("calculate md5 failed, file=[%s]", file_path.c_str());
        return false;
    }
    if (md5 == verify_data) {
        return true;
    } else {
        LEVIN_CWARNING_LOG(
                "check md5 failed, md5 unmatch, file=[%s] verify_data=[%s] calculate result=[%s]",
                file_path.c_str(),
                verify_data.c_str(),
                md5.c_str());
        return false;
    }
}

}
