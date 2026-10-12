#include <iostream>
#include <vector>
#include <pqxx/pqxx> // libpqxx が必要
#include <sstream>
#include <string>

using namespace std;

class PgSearch {
private:
    std::string name = "";

public:
    explicit PgSearch(std::string str){}

    std::vector<SearchDbItem> rag_search_items(
      std::vector<float> embedding, std::string pg_conn_str
    ) 
    {
      std::vector<SearchDbItem> items;
      std::string ret = "";
      try {
          pqxx::connection conn(pg_conn_str.c_str());
          if (conn.is_open()) {
              //cout << "Connected to database successfully." << endl;
          }else{
              cout << "error, not Connected to database" << endl;
          }

          pqxx::work txn(conn);
          vector<float> query_vector = embedding;

          stringstream ss;
          ss << "[";
          for (size_t i = 0; i < query_vector.size(); ++i) {
              if (i > 0) ss << ",";
              ss << query_vector[i];
          }
          ss << "]";
          string query_vec_str = ss.str();

          // 3. SQL実行 (<=> はコサイン距離)
          // 距離を "distance" として取得し、近い順(ASC)にソート
          std::string sql = "SELECT id, content, embedding <=> '" + query_vec_str + "'"
                            + " AS distance FROM documents ORDER BY distance LIMIT 3";
                            
          //cout << "sql=" << sql << "\n";
          pqxx::result result = txn.exec(sql);

          //cout << "Top similar vectors:\n";
          std::string matches = "";
          for (const auto& row : result) {
              SearchDbItem dbRow;
              dbRow.id = row["id"].as<long>();
              dbRow.distance = row["distance"].as<float>();
              dbRow.content = row["content"].c_str();
              if (dbRow.distance < 0.5) {
                  //matches = content;
                  items.push_back(dbRow);
              }
              //cout << "ID: " << id << ", distance: " << distance << endl;
          }
          txn.commit();

          //ret = matches;
          return items;
      } catch (const exception &e) {
          cerr << e.what() << endl;
          return items;
      }
      return items;
    }

    std::string pg_search(std::vector<float> embedding) {
      std::string ret = "";
      try {
          pqxx::connection conn("host=localhost user=root dbname=mydb password=admin");
          if (conn.is_open()) {
              //cout << "Connected to database successfully." << endl;
          }else{
              cout << "error, not Connected to database" << endl;
          }

          pqxx::work txn(conn);
          vector<float> query_vector = embedding;

          stringstream ss;
          ss << "[";
          for (size_t i = 0; i < query_vector.size(); ++i) {
              if (i > 0) ss << ",";
              ss << query_vector[i];
          }
          ss << "]";
          string query_vec_str = ss.str();

          // 3. SQL実行 (<=> はコサイン距離)
          // 距離を "distance" として取得し、近い順(ASC)にソート
          //string sql = "SELECT id, embedding , content FROM documents ORDER BY embedding <-> '" + query_vec_str + "' LIMIT 1;";
          std::string sql = "SELECT id, content, embedding <=> '" + query_vec_str + "'"
                            + " AS distance FROM documents ORDER BY distance LIMIT 1";
                            
          //cout << "sql=" << sql << "\n";
          pqxx::result result = txn.exec(sql);

          //cout << "Top similar vectors:\n";
          std::string matches = "";
          for (const auto& row : result) {
              int id = row["id"].as<int>();
              auto distance = row["distance"].as<float>();
              std::string content = row["content"].c_str();
              if (distance < 0.5) {
                  matches = content;
              }
              //cout << "ID: " << id << ", distance: " << distance << endl;
          }
          txn.commit();

          ret = matches;
          return ret;
      } catch (const exception &e) {
          cerr << e.what() << endl;
          return ret;
      }
      return ret;
    }

};
