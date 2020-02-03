#include <common.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

int main() {
  constexpr size_t num = 10;

  constexpr const char *files[num] = {
      "0822GUIJI.bin", "0823GUIJI.bin", "0824GUIJI.bin", "0825GUIJI.bin",
      "0826GUIJI.bin", "0827GUIJI.bin", "0828GUIJI.bin", "0829GUIJI.bin",
      "0830GUIJI.bin", "0831GUIJI.bin"};

  vector<unordered_map<int, vector<record>>> data(num);

#pragma omp parallel for
  for (size_t i = 0; i < 10; ++i) {
    auto file_name = string("/Users/shilei/Desktop/zhengzhou_csv/") + files[i];
    ifstream in(file_name, ios::binary);

    if (!in) {
      cerr << "Failed to open file " << file_name << endl;
      continue;
    }

    auto &local = data[i];

    uint32_t plate;
    uint32_t epoch;
    float lng;
    float lat;

    while (!in.eof()) {
      in.read(reinterpret_cast<char *>(&plate), sizeof(uint32_t));
      in.read(reinterpret_cast<char *>(&epoch), sizeof(uint32_t));
      in.read(reinterpret_cast<char *>(&lng), sizeof(float));
      in.read(reinterpret_cast<char *>(&lat), sizeof(float));

      local[plate].emplace_back(epoch, lng * 30, lat * 30);
    }
  }

  auto &target = data[0];

  for (size_t i = 1; i < 10; ++i) {
    for (auto &p : data[i]) {
      auto &item = target[p.first];
      item.insert(item.end(), p.second.cbegin(), p.second.cend());

      // Release the memory
      p.second.clear();
    }
  }

  ofstream out("/Users/shilei/Desktop/zhengzhou_csv/data.bin", ios::binary);

  for (const auto &p : target) {
    out.write(reinterpret_cast<const char *>(&(p.first)), sizeof(uint32_t));

    const uint32_t size = p.second.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));

    for (const auto &item : p.second) {
      out.write(reinterpret_cast<const char *>(&item.epoch), sizeof(uint32_t));
      out.write(reinterpret_cast<const char *>(&item.longitude), sizeof(float));
      out.write(reinterpret_cast<const char *>(&item.latitude), sizeof(float));
    }
  }

  out.close();

  return 0;
}
