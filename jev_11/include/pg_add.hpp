#pragma once

#include <iostream>
#include <vector>
#include <pqxx/pqxx> // libpqxx for PostgreSQL connection
#include <sstream>
#include <string>

using namespace std;

class PgAdd {
private:
    std::string name = "";
public:
    explicit PgAdd(std::string str){}

    void pg_add(
      std::vector<float> embedding, std::string content , std::string pg_conn_str
    ) {
        try {
            pqxx::connection conn(pg_conn_str.c_str());
            if (!conn.is_open()) {
                cerr << "Cannot connect to database" << endl;
                return;
            }

            pqxx::work txn(conn);

            stringstream ss;
            ss << "[";
            for (size_t i = 0; i < embedding.size(); ++i) {
                if (i > 0) ss << ",";
                ss << embedding[i];
            }
            ss << "]";

            string emb_str = ss.str();

            txn.exec_params(
                "INSERT INTO documents (embedding, content) VALUES ($1, $2)",
                emb_str, content
            );

            txn.commit();
            cout << "Vector inserted successfully!" << endl;

        } catch (const exception & e) {
            cerr << e.what() << endl;
        }
    }

    int pg_create(
      std::vector<float> embedding, std::string content, std::string pg_conn_str 
    ) 
    {
        int ret = 0;
        try {
            pqxx::connection conn(pg_conn_str.c_str());
            if (!conn.is_open()) {
                cerr << "Cannot connect to database" << endl;
                return ret;
            }

            pqxx::work txn(conn);

            stringstream ss;
            ss << "[";
            for (size_t i = 0; i < embedding.size(); ++i) {
                if (i > 0) ss << ",";
                ss << embedding[i];
            }
            ss << "]";

            string emb_str = ss.str();

            txn.exec_params(
                "INSERT INTO documents (embedding, content) VALUES ($1, $2)",
                emb_str, content
            );

            txn.commit();
            cout << "Vector inserted successfully!" << endl;
            return 1;
        } catch (const exception & e) {
            cerr << e.what() << endl;
        }
        return ret;
    }    

    ~PgAdd() {}  

};
