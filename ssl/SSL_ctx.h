// #ifdef OPENSSL_SUPPORT
#ifndef _SSL_CTX_
#define _SSL_CTX_
#include <unistd.h>
#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <memory>
 #include <sys/uio.h>  
#include <algorithm>
#include<iostream>
class SSL_ctx
{

private:
    std::string server_pem;
    std::string server_key;
    SSL_CTX *ctx;

public:
    // SSL_ctx(const char &SSL_ctx) = delete;
    SSL_ctx(std::string _server_pem, std::string _server_key) : server_pem(_server_pem), server_key(_server_key)
    {
       
        /* SSL 库初始化 */
        SSL_library_init();
        /* 载入所有 SSL 算法 */
        OpenSSL_add_all_algorithms();
        /* 载入所有 SSL 错误消息 */
        SSL_load_error_strings();
        /* 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text */
        ctx = SSL_CTX_new(SSLv23_server_method());
        /* 也可以用 SSLv2_server_method() 或 SSLv3_server_method() 单独表示 V2 或 V3标准 */
        if (ctx == NULL)
        {
            ERR_print_errors_fp(stdout);
            exit(1);
        }

        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
        // std::cout<<_server_pem;
        /* 载入用户的数字证书， 此证书用来发送给客户端。 证书里包含有公钥 */
        if (SSL_CTX_use_certificate_file(ctx, server_pem.c_str(), SSL_FILETYPE_PEM) != 1)
        {
            ERR_print_errors_fp(stdout);
            exit(1);
        }

        /* 载入用户私钥 */
        if (SSL_CTX_use_PrivateKey_file(ctx, server_key.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            ERR_print_errors_fp(stdout);
            exit(1);
        }

        /* 检查用户私钥是否正确 */
        if (!SSL_CTX_check_private_key(ctx))
        {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
      
    }

    static ssize_t SSL_readv(SSL *ssl, const  iovec *io_vector, int count)
    {
        char *buffer;
        char *bp;
        size_t bytes, to_copy;

        /* Find the total number of bytes to be written.  */
        bytes = 0;
        for (int i = 0; i < count; ++i)
            bytes += io_vector[i].iov_len;

        /* Allocate a temporary buffer to hold the data.  */
        buffer = (char *)alloca(bytes);

        /* Copy the data into BUFFER.  */
        to_copy = bytes;
        bp = buffer;
        for (int i = 0; i < count; ++i)
        {

            size_t copy = std::min(io_vector[i].iov_len, to_copy);

            memcpy((void *)bp, (void *)io_vector[i].iov_base, copy);
            bp += copy;

            to_copy -= copy;
            if (to_copy == 0)
                break;
        }

        return SSL_read(ssl, buffer, bytes);
    }

    static ssize_t SSL_writev(SSL *ssl, const  iovec *io_vector, int count)
    {
        
        char *bp;
        size_t bytes, to_copy;

        /* Find the total number of bytes to be written.  */
        bytes = 0;
        for (int i = 0; i < count; ++i)
            bytes += io_vector[i].iov_len;

        /* Allocate a temporary buffer to hold the data.  */
        std::unique_ptr<char> buffer(new char[bytes]);

        /* Copy the data into BUFFER.  */
        to_copy = bytes;
        bp = buffer.get();
        // std::cout<<bytes<<std::endl;
        for (int i = 0; i < count; ++i)
        {

            size_t copy = std::min(io_vector[i].iov_len, to_copy);

            memcpy((void *)bp, (void *)io_vector[i].iov_base, copy);
            bp += copy;

            to_copy -= copy;
            if (to_copy == 0)
                break;
        }

        return SSL_write(ssl, buffer.get(), bytes);
    }

    static void close_ssl(SSL *ssl, int client_fd)
    {
        if (ssl)
        {
            /* 关闭 SSL 连接 */
            SSL_shutdown(ssl);
            /* 释放 SSL */
            SSL_free(ssl);
        }

        if (client_fd > 0)
        {
            close(client_fd);
        }
    }

    SSL *SSL_new()
    {
        return ::SSL_new(ctx);
    }
    ~SSL_ctx()
    {
        std::cout<<"dead "<<std::endl;
        SSL_CTX_free(ctx);
    }
};

#endif
// #endif