#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct record {
  uint32_t epoch;
  float longitude;
  float latitude;

  record(uint32_t e, float lng, float lat)
      : epoch(e), longitude(lng), latitude(lat) {}
};

int main() {
  constexpr size_t num = 6;

  constexpr const char *files[num] = {"6-1.bin", "6-3.bin", "6-5.bin",
                                      "6-7.bin", "6-9.bin", "6-10.bin"};

  vector<unordered_map<string, vector<record>>> data(num);

#pragma omp parallel for
  for (size_t i = 0; i < num; ++i) {
    auto file_name =
        string("/Users/shilei/Desktop/haotian/wenzhou/") + files[i];
    ifstream in(file_name, ios::binary);

    if (!in) {
      cerr << "Failed to open file " << file_name << endl;
      continue;
    }

    auto &local = data[i];

    uint8_t len;
    uint32_t epoch;
    float lng;
    float lat;

    while (!in.eof()) {
      in.read(reinterpret_cast<char *>(&len), sizeof(uint8_t));

      vector<char> buf(len + 1);
      in.read(buf.data(), len);
      buf[len] = 0;

      string plate(buf.data());

      in.read(reinterpret_cast<char *>(&epoch), sizeof(uint32_t));
      in.read(reinterpret_cast<char *>(&lng), sizeof(float));
      in.read(reinterpret_cast<char *>(&lat), sizeof(float));

      local[plate].emplace_back(epoch, lng, lat);
    }
  }

  auto &target = data[0];

  for (size_t i = 1; i < num; ++i) {
    for (auto &p : data[i]) {
      auto &item = target[p.first];
      item.insert(item.end(), p.second.cbegin(), p.second.cend());

      // Release the memory
      p.second.clear();
    }
  }

  ofstream out("/Users/shilei/Desktop/haotian/wenzhou/data.bin", ios::binary);

  for (const auto &p : target) {
    const uint8_t len = p.first.size();
    out.write(reinterpret_cast<const char *>(&len), sizeof(uint8_t));

    out.write(reinterpret_cast<const char *>(p.first.data()), len);

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
