#ifndef TST_PARSER_UNIT_TESTS_H
#define TST_PARSER_UNIT_TESTS_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "parser.h"

namespace parser_unit_tests
{
extern const char* valid_path_to_ini_file;

class Config_searching_testing : public testing::Test
{
protected:
    Config_searching c_s_default;
    Config_searching c_s_c_string{"pet_project_config.ini"};
    Config_searching c_s_std_string{std::string("pet_project_config.ini")};
};

//Parameterizing by type
template<class T>
std::shared_ptr<Parser> create_parser(const char* path_c_string = nullptr, std::string path_std_string = std::string{})
{
    if (path_c_string != nullptr && path_std_string.empty())
    {
        return std::make_shared<T>(path_c_string);
    }
    if (path_c_string == nullptr && !path_std_string.empty())
    {
        return std::make_shared<T>(path_std_string);
    }
    return std::make_shared<T>();
}

//An order of char* returned by Parser_DB
enum p_DB : size_t
{
    HOST,
    HOSTADDR,
    PORT_DB,
    DBNAME,
    PASSWORD,
    CONNECT_TIMEOUT,
    CLIENT_ENCODING,
    SSLMODE
};

//An order of char* returned by Parser_Inotify
enum p_Inotify : size_t
{
    FILES_FOLDER_Inotify,
    LOGS_FOLDER_Inotify,
    MAX_LOG_FILE_SIZE_Inotify
};

//An order of char* returned by Parser_Server_HTTP
enum p_Server_HTTP : size_t
{
    ADDRESS,
    PORT_Server_HTTP,
    SERVER_NAME,
    FILES_FOLDER_Server_HTTP,
    LOGS_FOLDER_Server_HTTP,
    MAX_LOG_FILE_SIZE_Server_HTTP,
    NUM_THREADS_Server_HTTP,
    DAYS_LIMIT,
    ROWS_LIMIT
};

//An order of char* returned by Parser_Client_HTTP
enum p_Client_HTTP : size_t
{
    HOST_ADDRESS,
    PORT_SERVICE,
    CLIENT_NAME,
    VERSION,
    NUM_THREADS_Client_HTTP,
    DOWNLOAD_DIR,
    TARGETS
};

template<class T>
void keys_checking(const char* const* ptr_k)
{
    EXPECT_EQ(nullptr, ptr_k);
}

template<>
void keys_checking<Parser_DB>(const char* const* ptr_k)
{
    EXPECT_STREQ("host", ptr_k[HOST]);
    EXPECT_STREQ("hostaddr", ptr_k[HOSTADDR]);
    EXPECT_STREQ("port", ptr_k[PORT_DB]);
    EXPECT_STREQ("dbname", ptr_k[DBNAME]);
    EXPECT_STREQ("password", ptr_k[PASSWORD]);
    EXPECT_STREQ("connect_timeout", ptr_k[CONNECT_TIMEOUT]);
    EXPECT_STREQ("client_encoding", ptr_k[CLIENT_ENCODING]);
    EXPECT_STREQ("sslmode", ptr_k[SSLMODE]);
}

template<class T>
void values_checking(const char* const*);

template<>
void values_checking<Parser_DB>(const char* const* ptr_v)
{
    EXPECT_STREQ("localhost", ptr_v[HOST]);
    EXPECT_STREQ("127.0.0.1", ptr_v[HOSTADDR]);
    EXPECT_STREQ("5432", ptr_v[PORT_DB]);
    EXPECT_STREQ("pet_project_db", ptr_v[DBNAME]);
    EXPECT_STREQ("pet_project_password", ptr_v[PASSWORD]);
    EXPECT_STREQ("", ptr_v[CONNECT_TIMEOUT]);
    EXPECT_STREQ("auto", ptr_v[CLIENT_ENCODING]);
    EXPECT_STREQ("prefer", ptr_v[SSLMODE]);
}

template<>
void values_checking<Parser_Inotify>(const char* const* ptr_v)
{
    EXPECT_STREQ("/home/nikita/C++/Pet_project/songs_folder", ptr_v[FILES_FOLDER_Inotify]);
    EXPECT_STREQ("/home/nikita/C++/Pet_project/logs_folder/inotify_logs", ptr_v[LOGS_FOLDER_Inotify]);
    EXPECT_STREQ("1500", ptr_v[MAX_LOG_FILE_SIZE_Inotify]);
}

template<>
void values_checking<Parser_Server_HTTP>(const char* const* ptr_v)
{
    EXPECT_STREQ("0.0.0.0", ptr_v[ADDRESS]);
    EXPECT_STREQ("8080", ptr_v[PORT_Server_HTTP]);
    EXPECT_STREQ("pet_project_server", ptr_v[SERVER_NAME]);
    EXPECT_STREQ("/home/nikita/C++/Pet_project/songs_folder", ptr_v[FILES_FOLDER_Server_HTTP]);
    EXPECT_STREQ("/home/nikita/C++/Pet_project/logs_folder/server_http_logs", ptr_v[LOGS_FOLDER_Server_HTTP]);
    EXPECT_STREQ("1500", ptr_v[MAX_LOG_FILE_SIZE_Server_HTTP]);
    EXPECT_STREQ("2", ptr_v[NUM_THREADS_Server_HTTP]);
    EXPECT_STREQ("1", ptr_v[DAYS_LIMIT]);
    EXPECT_STREQ("10", ptr_v[ROWS_LIMIT]);
}

template<>
void values_checking<Parser_Client_HTTP>(const char* const* ptr_v)
{
    EXPECT_STREQ("127.0.0.41, 127.0.0.42, 127.0.0.43, 127.0.0.44, 127.0.0.45, 127.0.0.46, 127.0.0.47, 127.0.0.48, "
                                                                                                "127.0.0.49", ptr_v[HOST_ADDRESS]);
    EXPECT_STREQ("8080, 8080, 8080, 8080, 8080, 8080, 8080, 8080, 8080", ptr_v[PORT_SERVICE]);
    EXPECT_STREQ("pet_project_client", ptr_v[CLIENT_NAME]);
    EXPECT_STREQ("", ptr_v[VERSION]);
    EXPECT_STREQ("2", ptr_v[NUM_THREADS_Client_HTTP]);
    EXPECT_STREQ("/home/nikita/C++/Pet_project/Download/1, /home/nikita/C++/Pet_project/Download/2", ptr_v[DOWNLOAD_DIR]);
    EXPECT_STREQ("files_table, log_table, 11, 11.wav, 14.wav, 15, 25.mp3, 35.wav, 3.mp3, 53.mp3, 63.mp3, added2.mp3, song1.mp3, "
                                                                                                        "song2.wav", ptr_v[TARGETS]);
}

template<class T>
class Parser_testing : public testing::Test
{
protected:
    std::shared_ptr<Parser> parser{nullptr};
    Parser_testing(const char* path_c_string = nullptr, std::string path_std_string = std::string{}):
                                                                        parser(create_parser<T>(path_c_string, path_std_string)) { };
    ~Parser_testing() override { }
    void keys_values_checking()
    {
        //Keys checking
        const char* const* ptr = parser->parsed_info_ptr();
        keys_checking<T>(ptr);
        const char* const* ptr_k = parser->parsed_info_ptr('k');
        keys_checking<T>(ptr_k);
        const char* const* ptr_K = parser->parsed_info_ptr('K');
        keys_checking<T>(ptr_K);
        //Values checking
        const char* const* ptr_v = parser->parsed_info_ptr('v');
        values_checking<T>(ptr_v);
        const char* const* ptr_V = parser->parsed_info_ptr('v');
        values_checking<T>(ptr_V);
    }
};

template<class T>
class Parser_default_ctor : public Parser_testing<T>
{
protected:
    Parser_default_ctor(): Parser_testing<T>() { }
};

template<class T>
class Parser_C_string_ctor : public Parser_testing<T>
{
protected:
    Parser_C_string_ctor(): Parser_testing<T>(valid_path_to_ini_file) { }
};

template<class T>
class Parser_std_string_ctor : public Parser_testing<T>
{
protected:
    Parser_std_string_ctor(): Parser_testing<T>(nullptr, std::string{valid_path_to_ini_file}) { }
};

using ::testing::Types;

typedef Types<Parser_DB, Parser_Inotify, Parser_Server_HTTP, Parser_Client_HTTP> Implementations;

TYPED_TEST_SUITE(Parser_default_ctor, Implementations);
TYPED_TEST_SUITE(Parser_C_string_ctor, Implementations);
TYPED_TEST_SUITE(Parser_std_string_ctor, Implementations);


/*class Parser_DB_testing : public testing::Test
{
protected:
    Parser_DB parser_constructed{"/home/nikita/C++/Pet_project/ini_and_parser/pet_project_config.ini"};
    void keys_checking(const char* const* ptr_k)
    {
        EXPECT_STREQ("host", ptr_k[0]);
        EXPECT_STREQ("hostaddr", ptr_k[1]);
        EXPECT_STREQ("port", ptr_k[2]);
        EXPECT_STREQ("dbname", ptr_k[3]);
        EXPECT_STREQ("password", ptr_k[4]);
        EXPECT_STREQ("connect_timeout", ptr_k[5]);
        EXPECT_STREQ("client_encoding", ptr_k[6]);
        EXPECT_STREQ("sslmode", ptr_k[7]);
    }
    void values_checking(const char* const* ptr_v)
    {
        EXPECT_STREQ("localhost", ptr_v[0]);
        EXPECT_STREQ("127.0.0.1", ptr_v[1]);
        EXPECT_STREQ("5432", ptr_v[2]);
        EXPECT_STREQ("pet_project_db", ptr_v[3]);
        EXPECT_STREQ("pet_project_password", ptr_v[4]);
        EXPECT_STREQ("", ptr_v[5]);
        EXPECT_STREQ("auto", ptr_v[6]);
        EXPECT_STREQ("prefer", ptr_v[7]);
    }
    void p_DB_checking(const Parser_DB& p_DB)
    {
        //Keys checking
        const char* const* ptr_ = p_DB.parsed_info_ptr();
        keys_checking(ptr_);
        const char* const* ptr_k = p_DB.parsed_info_ptr('k');
        keys_checking(ptr_k);
        const char* const* ptr_K = p_DB.parsed_info_ptr('K');
        keys_checking(ptr_K);
        //Values checking
        const char* const* ptr_v = p_DB.parsed_info_ptr('v');
        values_checking(ptr_v);
        const char* const* ptr_V = p_DB.parsed_info_ptr('v');
        values_checking(ptr_V);
    }
};*/
}   //namespace parser_unit_tests

#endif // TST_PARSER_UNIT_TESTS_H
