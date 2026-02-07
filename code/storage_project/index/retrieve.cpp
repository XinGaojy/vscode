/********************************************************************************
 * @File        : inv_index.h
 * @Author      : Shard Zhang
 * @Date        : 2023/10/22
 * @Brief       : 计算广告中的倒排索引代码示例
 ********************************************************************************/
#ifndef CPP_BOX_INV_INDEX_H
#define CPP_BOX_INV_INDEX_H

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;
using std::string;
using std::map;
using std::vector;
using std::pair;

namespace inv_index {
typedef pair<int, double> Entry; // pair: <doc_id, tf_idf>

template<class Key>
class InvIndex : public map<Key, vector<Entry>> {
 public:
  vector<vector<Key>> docs; // 正排表
  void add(const vector<Key> &doc) {
      // 在正排表里记录该文档
      docs.push_back(doc);
      int cur_doc_id = docs.size();

      // 遍历document里所有的term
      for (int w = 0; w < doc.size(); ++w) {
          // 如果该term的倒排链不存在
          if (!this->count(doc[w])) {
              this->insert({doc[w], {}});
          }
          // 在倒排链末尾加入新的文档ID
          (*this)[doc[w]].push_back({cur_doc_id, 0.0});
      }
  }

  void retrieve(const vector<Key> &query, vector<int> &doc_ids) {
      int term_num = query.size();

      // 合并所有term的倒排链
      vector<Entry> result;
      for (int t = 0; t < term_num; ++t) {
          // 该term倒排链不存在则跳过
          if (!this->count(query[t])) {
              continue;
          }
          for (auto entry : (*this)[query[t]]) {
              result.push_back(entry);
          }
      }

      // 得到返回的文档ID集合
      doc_ids.clear();
      for (auto entry : result) {
          doc_ids.push_back(entry.first);
      }
  }

  void print() const {
      for (const auto& e : *this) {
          std::cout << e.first << " -> ";
          for (auto entry : e.second) {
              std::cout << entry.first << " ";
          }
          std::cout << std::endl;
      }
  }
};

int run() {
    vector<string> d1 = {"谷歌", "地图", "之父", "跳槽", "Facebook"};
    vector<string> d2 = {"谷歌", "地图", "之父", "加盟", "Facebook"};
    vector<string> d3 = {"谷歌", "地图", "创始人", "拉斯", "离开", "谷歌", "加盟", "Facebook"};
    vector<string> d4 = {"谷歌", "地图", "创始人", "跳槽", "Facebook", "Wave", "项目", "取消", "有关"};
    vector<string> d5 = {"谷歌", "地图", "创始人", "拉斯", "加盟", "社交", "网站", "Facebook"};

    InvIndex<string> invdex;
    invdex.add(d1);
    invdex.add(d2);
    invdex.add(d3);
    invdex.add(d4);
    invdex.add(d5);
    invdex.print();

    vector<int> result;
    vector<string> query = {"拉斯", "Wave"};
    invdex.retrieve(query, result);

    printf("query result: ");
    for (auto id : result) {
        printf("%d ", id);
    }
    return 0;
}
}
#endif //CPP_BOX_INV_INDEX_H


int main(){
    inv_index::run();
    return 0;
}
