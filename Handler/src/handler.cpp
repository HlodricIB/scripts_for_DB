#include <functional>
#include <algorithm>
//#include <boost/beast/core/string_type.hpp>
#include "handler.h"

bool Inotify_DB_handler::handle(std::vector<std::string>& arguments)
{
    std::string command;
    if (arguments[0] == "add")
    {
        size_t uid = std::hash<std::string>{}(arguments[1]);
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                "INSERT INTO song_table (id, song_name, song_uid, song_URL) VALUES (DEFAULT, "
                + arguments[1] + ", " + '\'' + std::to_string(uid) + '\'' + ", " + '\'' + files_folder + '\'' + "); COMMIT";
        exec_command(command);
        return true;
    }
    if (arguments[0] == "delete")
    {
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                "DELETE FROM song_table WHERE song_name = '" + arguments[1] + "'; COMMIT";
        exec_command(command);
        return true;
    }
    if (arguments[0] == "dir_del")
    {
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; TRUNCATE song_table RESTART IDENTITY; COMMIT";
        exec_command(command);
        return true;
    }
    if (arguments[0] == "refresh")
    {
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; TRUNCATE song_table RESTART IDENTITY; COMMIT";
        auto future_res = DB_ptr->exec_command(command, true);   //Clearing previous song list
        auto res = future_res.get();    //Have to block here to avoid possibility of truncating subsequent addings
        if (!res->res_succeed())
        {
            std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
            return false;
        }
        auto amount = arguments.size();
        std::string add("add");
        for (std::vector<std::string>::size_type i = 1; i != amount; ++i)
        {
            std::vector<std::string> temp{add, arguments[i]};
            handle(temp);
        }
        return true;
    }
    return false;
}

void Inotify_DB_handler::exec_command(const std::string& command)
{
    auto future_res = DB_ptr->exec_command(command, true);
    auto lambda = [future_res = std::move(future_res), command] () mutable {
        auto res = future_res.get();
        if (res->res_succeed())
        {
            return;
        } else
        {
            std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
            return;
        }
    };
    t_pool_ptr->push_task_back(std::move(lambda));
    return;
}

//An order of parameters in req_info
enum : size_t
{
    REQ_TARGET,
    REQ_HOST,
    REQ_PORT,
    REQ_IP_ADDRESS,
    REQ_METHOD,
    REQ_RESULT,
    REQ_ERROR
};

void Server_HTTP_handler::update_log_table(std::vector<std::string>& req_info)
{
    static std::string command;
    command = "BEGIN; LOCK TABLE log_table IN ACCESS EXCLUSIVE MODE; "
                "INSERT INTO log_table (id, host, port, ip, rest_method, target, req_date_time) VALUES"
                "(DEFAULT, \'" +
                req_info[REQ_HOST] + "\', \'" +
                req_info[REQ_PORT] + "\'::serial, \'" +
                req_info[REQ_IP_ADDRESS] + "\'::inet, \'" +
                req_info[REQ_METHOD] + "\', \'" +
                req_info[REQ_TARGET] + "\', \'" +
                "LOCALTIMESTAMP" +
                "); COMMIT";
    auto future_res = DB_ptr->exec_command(command, false);  //Second argument (false) tells, that command must be queued in a
                                                             //task_deque of appropriate thread pool that works with DB_module
    auto res = future_res.get();
    if (!res->res_succeed())
    {
        std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
        req_info[REQ_ERROR] += std::string("Database " + res->res_DB_name() + " error while updating log table: " + res->res_error() + '\n');
    }
}

void Server_HTTP_handler::forming_files_table(std::vector<std::string>& req_info)
{
    static std::string command{"BEGIN; LOCK TABLE song_table IN SHARE MODE; SELECT id, song_name, song_uid FROM song_table; COMMIT"};
    auto future_res = DB_ptr->exec_command(command, false);  //Second argument (false) tells, that command must be queued in a
                                                             //task_deque of appropriate thread pool that works with DB_module
    auto res = future_res.get();
    if (!res->res_succeed())
    {
        std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
        req_info[REQ_ERROR].append(std::string("Database " + res->res_DB_name() + " error while forming files table: " + res->res_error() + '\n'));
        return;
    }
    if (res->get_columns_number() == 0)
    {
        req_info[REQ_RESULT] = "No files available at that moment\n";
        return;
    }
    result_container res_cntnr(res->get_result_container());
    forming_tables_helper(req_info, res_cntnr);
}

void Server_HTTP_handler::forming_log_table(std::vector<std::string>& req_info)
{
    static std::string command{"BEGIN; LOCK TABLE log_table IN SHARE MODE; SELECT * FROM log_table; COMMIT"};
    auto future_res = DB_ptr->exec_command(command, false);  //Second argument (false) tells, that command must be queued in a
                                                             //task_deque of appropriate thread pool that works with DB_module
    auto res = future_res.get();
    if (!res->res_succeed())
    {
        std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
        req_info[REQ_ERROR].append(std::string("Database " + res->res_DB_name() + " error while forming log table: " + res->res_error() + '\n'));
        return;
    }
    result_container res_cntnr(res->get_result_container());
    forming_tables_helper(req_info, res_cntnr);
}

void Server_HTTP_handler::forming_tables_helper(std::vector<std::string>& req_info, result_container& res_cntnr)
{
    //Determining maximum length of strings in every column exclude last column (for formatted table sending)
    auto n_columns = res_cntnr[0].size() - 1;  //-1 here is because of we don't need to take into account
                                               //lenght of rows in the last column that would be sent in response
    std::vector<size_t> vec_max_len(n_columns, 0); //
    for (inner_result_container::size_type i = 0; i != n_columns; ++i)
    {
        for (result_container::size_type j = 0; j != res_cntnr.size(); ++j)
        {
            vec_max_len[i] = std::max<size_t>(vec_max_len[i], res_cntnr[j][i].second);
        }
    }
    //Forming files table in std::string
    ++n_columns;   //All columns must be present in the result string
    for (result_container::size_type i = 0; i != res_cntnr.size(); ++i)
    {
        for (inner_result_container::size_type j = 0; j != n_columns; ++j)
        {
            req_info[REQ_RESULT] += res_cntnr[i][j].first + std::string(vec_max_len[j] - res_cntnr[i][j].second + 1, ' ');
        }
        req_info[REQ_RESULT].append(1, '\n');
    }
}

void Server_HTTP_handler::get_file_URI(std::vector<std::string>& req_info)
{
    auto& target = req_info[REQ_TARGET];
    if (target.back() == '/' || target.find("..") != std::string::npos)
    {
        req_info[REQ_ERROR] = "Illegal request-target\n";  //Fill target_body_info[REQ_ERROR] element (element for error)
        return;
    }

    if (target.size() > 1 && target.back() == '*')
    {
        target.back() = '%';
    }
    static std::string command_name{"SELECT song_name, song_url FROM song_table WHERE song_name SIMILAR TO " + target + " LIMIT 1;"};
    static std::string command_uid{"SELECT song_name, song_url FROM song_table WHERE song_uid SIMILAR TO " + target + " LIMIT 1;"};
    auto future_res = DB_ptr->exec_command(command_name, false);  //Second argument (false) tells, that command must be queued in a
                                                                       //task_deque of appropriate thread pool that works with DB_module

    auto res = future_res.get();
    if (!res->res_succeed())
    {
        std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
        req_info[REQ_ERROR].append(std::string("Database " + res->res_DB_name() + " error while forming log table: " + res->res_error() + '\n'));
        return;
    }
    future_res = DB_ptr->exec_command(command_uid, false);
    result_container res_cntnr(res->get_result_container());
}

//"SELECT song_name, song_url FROM song_table WHERE song_name SIMILAR TO 'rt%?' LIMIT 1;"

bool Server_HTTP_handler::handle(std::vector<std::string>& req_info)    //Returning bool value shows if we have request for file (true) or
                                                                        //request for something else (false)
                                                                        //First element is requested target, second - host, third - port, fourth - ip
                                                                        //fifth - method, sixth - is for storing result of handling by Handler,
                                                                        //seventh - is for storing possible error message
{
    auto& target = req_info[REQ_TARGET];
    update_log_table(req_info);
    //Check if we have a request for both, files and log tables
    if (target.empty() || (target.size() == 1 && target[0] == '/'))
    {
        forming_files_table(req_info);
        forming_log_table(req_info);
        return false;
    }
    //Check if we have a request for files table
    if (target == "files_table" || target == "Files_table")
    {
        forming_files_table(req_info);
        return false;
    }
    //Check if we have a request for logs table
    if (target == "log_table" || target == "Log_table")
    {
        forming_files_table(req_info);
        return false;
    }
    //If we are here, we have a request for file
    get_file_URI(req_info);
    return true;
}

void Server_dir_handler::check_dir()
{
    std::error_code ec;
    std::string_view msg;
    bool _is_directory = std::filesystem::is_directory(files_path, ec);
    if (ec || !_is_directory)
    {
        if (ec)
        {
            msg = "Unable to check if given path to files dir is directory: " + ec.message();
        } else {
            msg = "Given path to files dir is not a directory";
        }
        std::cerr << msg << std::endl;
        files_path.clear();
        check_res = false;
        return;
    }
    check_res = true;
}

void Server_dir_handler::find(std::vector<std::string>& req_info)
{
    auto& target = req_info[REQ_TARGET];
    if (target.back() == '/' || target.find("..") != std::string::npos)
    {
        req_info[REQ_ERROR] = "Illegal request-target\n";  //Fill target_body_info[REQ_ERROR] element (element for error)
        return;
    }
    static std::function<bool(std::string_view)> compare;
    if (target.size() > 1 && target.back() == '*')
    {
        target.resize(target.size() - 1);
        compare = [&target] (std::string_view curr_file) mutable {
            if (curr_file.starts_with(target))
            {
                target = curr_file;
                return true;
            }
            return false; };
    } else {
        compare = [target] (std::string_view curr_file)->bool { return curr_file == target; };
    }
    std::string_view curr_file;
    for (auto const& dir_entry : std::filesystem::directory_iterator{files_path})
    {
        if (dir_entry.is_regular_file())
        {
            curr_file = dir_entry.path().filename().c_str();
            if (compare(curr_file))
            {
                std::filesystem::path temp_filename{curr_file};
                auto temp_files_path = files_path;
                req_info[REQ_RESULT] = (temp_files_path /= temp_filename).string();
                return;
            }
        }
    }
    req_info[REQ_ERROR] = "File " + target + " not found\n";  //Fill target_body_info[REQ_ERROR] element (element for error)
    return;
}

std::vector<std::string>::size_type Server_dir_handler::number_of_digits(std::vector<std::string>::size_type n)
{
    std::vector<std::string>::size_type number_of_digits = 0;
    do
    {
        ++number_of_digits;
        n /= 10;
    } while (n != 0);
    return number_of_digits;
}

std::string Server_dir_handler::forming_files_table()
{
    std::vector<std::string> file_names;    //Temporary container to store all file names in specified dir;
    std::string::size_type max_file_length = 0;
    std::vector<std::string>::size_type i = 0;
    for (auto const& dir_entry : std::filesystem::directory_iterator{files_path})
    {
        if (dir_entry.is_regular_file())
        {
            file_names.push_back(dir_entry.path().filename().string());
            max_file_length = std::max(max_file_length, file_names[i].size());
            ++i;
        }
    }
    //Determine maximum number of digits in maximum number of files integer (for formatted string_body)
    auto max_digits = number_of_digits(i);
    //Max length of record in first column
    auto max_length_first_column = std::max<std::vector<std::string>::size_type>(max_digits, 3);  //3 is the length of first column
                                                                                                    //name (Pos)
    auto num_spaces = max_length_first_column - 3 + 1;
    std::string files_table = "Pos" + std::string(num_spaces, ' ') + "File name\n";
    for (std::vector<std::string>::size_type c = 0, pos = 1; c != i; ++c, ++pos)
    {
        num_spaces = max_length_first_column - number_of_digits(pos) + 1;
        files_table += std::to_string(pos) + std::string(num_spaces, ' ') + file_names[c] + '\n';
    }
    return files_table;
}

bool Server_dir_handler::set_path(const char* files_path_)
{
    files_path = files_path_;
    check_dir();
    return check_res;
}

bool Server_dir_handler::handle(std::vector<std::string>& req_info) //First element is requested target, second - host, third - port, fourth - ip
                                                                    //fifth - method, sixth - is for storing result of handling by Handler,
                                                                    //seventh - is for stroring possible error message
{
    auto& target = req_info[REQ_TARGET];
    //Check if we have a request for files table
    if (target.empty() || (target.size() == 1 && target[0] == '/') || target == "files_table" || target == "Files_table")
    {
        req_info[REQ_RESULT] = forming_files_table();
        return false;   //Shows that we have request for something else, but not the file
    } else {
        find(req_info);
        return true;    //Shows that we have request for file
    }
}
