#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <regex>
#include <string>
#include <fstream>

#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"
#include "../log/log.h"

class http_conn
{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

private:
    enum FORMDATA_STAGE
    {
        FORMDATA_STAGE_BEGIN = 0,
        FORMDATA_STAGE_ATTRI,
        FORMDATA_STAGE_ATTRI_B,
        FORMDATA_STAGE_FILE
    };

    enum FORMDATA_CODE
    {
        FORMDATA_CODE_CONTINUTE = 0,
        FORMDATA_CODE_CONTINUTE_SEGMENT,
        FORMDATA_CODE_END,
        FORMDATA_CODE_BAD
    };

public:
    http_conn() {}
    ~http_conn() {}

public:
    void init(int sockfd, const sockaddr_in &addr, char *, int, int, string user, string passwd, string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in *get_address()
    {
        return &m_address;
    }
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv;


private:
    void init();
    HTTP_CODE process_read();
    bool process_write(HTTP_CODE ret);
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char *get_line() { return m_read_buf + m_start_line; };
    LINE_STATUS parse_line();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    bool parse_header(char *line);
    bool is_formdata();
    vector<string> split(const string &str, const string &pattern);
    http_conn::FORMDATA_CODE parse_content_formdata(char *text);
    bool is_permissible();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

private:
    int m_sockfd;
    sockaddr_in m_address;
    char m_read_buf[READ_BUFFER_SIZE + 10]; //这里加10，是为了防止数据包刚好为READ_BUFFER_SIZE时，后面有位置存'\0'
    //char m_file_buf[READ_BUFFER_SIZE];
    long m_read_idx;    //缓冲区接收到的数据长度
    long m_checked_idx; //缓冲区被查询的数据序号
    long m_read_file_start;    //缓冲区文件开始
    long m_read_file_end;    //缓冲区文件结束
    int m_start_line;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;
    CHECK_STATE m_check_state;
    METHOD m_method;
    char m_real_file[FILENAME_LEN]; //存储文件名
    char *m_url;
    char *m_version;
    char *m_host;
    long m_content_length;  //请求体长度
    long m_content_remain;  //请求体剩余接收长度
    bool m_linger;
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    int cgi;        //是否启用的POST
    char *m_string; //存储请求头数据
    int bytes_to_send;
    int bytes_have_send;
    char *doc_root;
    map<string, string> headers;
    string boundary;
    string filename;
    long file_length;
    bool isformdata;
    FORMDATA_STAGE formdata_stage;
    map<string, string> formdata_attri;
    FORMDATA_CODE formdata_code;
    bool recv_segment;  //false: 完整http包；true：不完整http包
    bool parse_by_line; //false: 不要逐行解析；true：逐行解析
    ofstream ofs;

    map<string, string> m_users;
    int m_TRIGMode;
    int m_close_log;

    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
