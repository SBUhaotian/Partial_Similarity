#include <common.hpp>

#include <fstream>
#include <iostream>
#include <random>
#include <unordered_set>

using namespace std;

shared_ptr<vector<vector<record>>> read_data(const string &file_name) {
  ifstream in(file_name, ios::binary);

  if (!in) {
    cerr << "Failed to open data file" << endl;
    return nullptr;
  }

  auto data = make_shared<vector<vector<record>>>();

  while (!in.eof() && in.tellg() >= 0) {
    // Skip plate number cause we don't need it in this case
#ifdef WENZHOU
    uint8_t plate_len;
    in.read(reinterpret_cast<char *>(&plate_len), sizeof(uint8_t));
    in.seekg(plate_len, ios::cur);
#else
    in.seekg(sizeof(uint32_t), ios::cur);
#endif

    uint32_t num;

    in.read(reinterpret_cast<char *>(&num), sizeof(uint32_t));

    if (num < size_threshold) {
      in.seekg(num * 3 * 4, ios::cur);
      continue;
    }

    vector<record> records;

    uint32_t epoch;
    float lng, lat;

    for (size_t i = 0; i < num; ++i) {
      in.read(reinterpret_cast<char *>(&epoch), sizeof(uint32_t));
      in.read(reinterpret_cast<char *>(&lng), sizeof(float));
      in.read(reinterpret_cast<char *>(&lat), sizeof(float));

      epoch /= time_slot;

      // The data of Zhengzhou requires some extra process to divide the whole
      // area into `grid_size * grid_size` grids.
#ifndef WENZHOU
      lng = (lng - lng_l) / lng_range * grid_size;
      lat = (lat - lat_d) / lat_range * grid_size;
#endif

      records.emplace_back(epoch, lng, lat);
    }

    data->push_back(move(records));
  }

  return data;
}

#ifdef WENZHOU
static unordered_map<pair<float, float>, uint8_t>
get_bs_map(const shared_ptr<vector<vector<record>>> &data) {
  unordered_map<pair<float, float>, uint8_t> map;

  for (const auto &t : *data) {
    for (const auto &r : t) {
      map[make_pair(r.longitude, r.latitude)] = 0;
    }
  }

  return map;
}

shared_ptr<vector<vector<record>>>
sample_data(const shared_ptr<vector<vector<record>>> &data, const float p) {
  unordered_map<uint32_t, unordered_map<pair<float, float>, uint8_t>> masks;

  const auto map_template = get_bs_map(data);

  random_device rd;
  mt19937 gen(rd());
  bernoulli_distribution bd(p);

  auto new_data = make_shared<vector<vector<record>>>();

  for (auto &t : *data) {
    vector<record> nt;

    for (auto &r : t) {
      if (masks.find(r.epoch) == masks.end()) {
        masks[r.epoch] = map_template;
        // Generate random mask
        for (auto &item : masks[r.epoch]) {
          item.second = bd(gen);
        }
      }

      if (masks[r.epoch][make_pair(r.longitude, r.latitude)]) {
        nt.push_back(r);
      }
    }

    new_data->push_back(move(nt));
  }

  return new_data;
}
#else
shared_ptr<vector<vector<record>>>
sample_data(const shared_ptr<vector<vector<record>>> &data, const float p) {
  unordered_map<uint32_t, vector<vector<uint8_t>>> masks;

  random_device rd;
  mt19937 gen(rd());
  bernoulli_distribution bd(p);

  auto new_data = make_shared<vector<vector<record>>>();

  for (auto &t : *data) {
    vector<record> nt;

    for (auto &r : t) {
      if (masks.find(r.epoch) == masks.end()) {
        // Init the masks
        masks.emplace(make_pair(
            r.epoch,
            vector<vector<uint8_t>>(grid_size, vector<uint8_t>(grid_size))));

        // Generate random mask
        for (auto &row : masks[r.epoch]) {
          for (auto &e : row) {
            e = bd(gen);
          }
        }
      }

      if (masks[r.epoch][r.longitude][r.latitude]) {
        nt.push_back(r);
      }
    }

    new_data->push_back(move(nt));
  }

  return new_data;
}
#endif
